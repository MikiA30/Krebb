# Krebb One firmware → iOS handoff

A ready-to-send summary of what the ESP32-S3 actually implements, for the software lead building the BLE client.

Two items are marked `<<FILL WITH REAL DATA FROM MY HARDWARE RUN>>`. They stay that way until the bench session in [test-plan-firmware.md](test-plan-firmware.md) has been run — no invented packets, no invented measurements.

---

## (a) Pin map

| Signal | ESP32-S3 GPIO | Notes |
| --- | --- | --- |
| DS18B20 DQ (skin contact) | **GPIO4** | 1-Wire, external 4.7 kOhm pull-up to 3V3, powered mode (not parasitic) |
| DHT11 DATA (ambient) | **GPIO5** | 3-pin module with its own pull-up; no external resistor |
| Sensor power | 3V3 | DevKitC-1 J1 pin 1, common GND on J3 pin 1 |

Board: ESP32-S3-DevKitC-1, ESP32-S3-WROOM-1-N8R8. PSRAM is not enabled. Full reasoning in [wiring/pin-map.md](wiring/pin-map.md). Nothing here affects the app — it is included so both sides have the same picture of the device.

## (b) UUIDs as implemented

| Item | Value |
| --- | --- |
| Device name (advertised) | `Krebb One` |
| Service UUID | `F000A001-0451-4000-B000-000000000000` |
| Measurement characteristic | `F000A002-0451-4000-B000-000000000000` |
| Properties | **NOTIFY only** (no read, no write) |

These match [docs/BLE_PROTOCOL.md](../docs/BLE_PROTOCOL.md) and [docs/sensor-protocol.md](../docs/sensor-protocol.md) exactly. Nothing was renamed or added.

**Discovery:** the 128-bit service UUID is in the **advertising data**; the device name is in the **scan response**. Scanning by service UUID works and is the recommended path — a 128-bit UUID takes 18 of the 31 advertising bytes, so the name would not reliably fit alongside it.

**Session model:** an active session is a connected central that has **subscribed** to notifications. While subscribed, the device notifies once per second. While not subscribed it sends nothing at all. Sensors keep sampling either way, so the quality window is already warm when the app subscribes.

**Disconnect:** advertising restarts automatically. Repeated connect / subscribe / disconnect / reconnect cycles are supported without a reboot.

**MTU:** the packet is roughly 100–115 bytes, which does **not** fit the default 23-byte ATT MTU. The device requests an MTU of 247. Before every notification it compares the packet length against (negotiated MTU − 3) and, if it would not fit, logs a loud error and **sends nothing** rather than a truncated packet. iOS negotiates a large MTU automatically, so this should not bite in practice — but if the app ever sees a packet stop rather than truncate, that is why.

## (c) Three example packets

`<<FILL WITH REAL DATA FROM MY HARDWARE RUN>>`

These will be three consecutive real `PKT` lines from a stable, clock-synced session. Until then, the **format** (not the values) is:

```json
{"timestampMs":1760000000000,"skinTemperatureC":33.42,"ambientTemperatureC":24.0,"sensorQuality":0.91}
```

and an unavailable skin reading looks like:

```json
{"timestampMs":1760000001000,"skinTemperatureC":null,"ambientTemperatureC":24.9,"sensorQuality":0.0}
```

Both are verified byte-exactly by the host unit tests. Guarantees the app can rely on:

- One compact single-line UTF-8 JSON object per notification. No trailing newline, no whitespace padding.
- Exactly those four keys, that casing. No humidity, no heart rate, no extra fields today.
- `timestampMs` is an integer and always present, never null.
- `skinTemperatureC` (2 decimals) and `ambientTemperatureC` (1 decimal) are a JSON number **or the literal `null`**.
- `sensorQuality` is always a number in `[0.0, 1.0]`, 2 decimals.
- Degrees Celsius throughout.
- `nan`, `inf` and the sensor sentinels (`-127`, `85.0`) can never appear — there is a hard guard in the serialiser plus tests for it.
- Numbers use `.` as the decimal separator regardless of locale (rendered from scaled integers, not `printf("%f")`).

**Please still tolerate unknown fields**, per the contract, so a future field does not need a firmware/app lockstep release.

## (d) Skin-stability note

`<<FILL WITH REAL DATA FROM MY HARDWARE RUN>>`

To be filled in after the baseline run with: whether the values are stable when attached to skin, how long quality takes to reach ≥ 0.70, the settled window range, and anything that reliably knocks contact loose.

What the app can expect structurally, independent of those numbers:

- After placement, readings drift for a while before settling. `sensorQuality` reports this as the `0.3–0.6` stabilizing band, which maps to the app's `stabilizing` state.
- Any invalid read resets the warmup, so quality drops to `0.0` and then re-enters the `0.3–0.6` band rather than jumping straight back to `0.7+`. A brief contact loss therefore costs about a minute of "ready" state — that is deliberate, not a glitch.
- The relevant signal is the **within-session trend from baseline**. The absolute value is not a body temperature and should not be shown as one.

## (e) Known limits and open questions

### 1. Time sync — needs a decision (blocking for real timestamps)

The contract requires `timestampMs` to be Unix milliseconds, but the ESP32-S3 has no RTC, no battery-backed clock, and no NTP here, and the protocol defines **no** sync mechanism. I did not add one, because that would change the shared contract unilaterally.

Today: the device falls back to `BUILD_UNIX_EPOCH_S * 1000 + millis()` (the build time), and reports `TIME=UNSYNCED(dev-fallback)` on serial until a real epoch is set with the `t <unix_ms>` serial command. **So device timestamps are currently correct in their spacing (exactly 1000 ms apart) but wrong in their absolute offset unless I set the clock by hand before a session.**

Options, with the contract change each needs — my recommendation is option B for the hackathon:

| Option | What changes | Cost |
| --- | --- | --- |
| **A. Phone writes the time on connect** | Add one writable characteristic to the service; document it in `BLE_PROTOCOL.md` and `sensor-protocol.md` | Device timestamps become genuinely correct. Needs a new UUID agreed by both of us plus firmware and app work. Most correct, largest change. |
| **B. App stamps arrival time (recommended)** | No packet change. Document that `timestampMs` is a **device-relative monotonic** clock and that the app records its own arrival time alongside | Zero contract risk, works today. The app already has to record a session start; relative spacing is what feature extraction actually needs. Cost: device timestamps are not wall-clock until synced. |
| **C. Wi-Fi NTP on the device** | No contract change | Not recommended: needs credentials on the device, adds a radio that competes with BLE, and fails at a venue with captive-portal Wi-Fi. |

If you pick B, I suggest we add one line to `docs/sensor-protocol.md` saying `timestampMs` is device time and may be unsynced. That is a documentation change for both of us to agree — I have not touched `docs/`.

### 2. Ambient freshness rule — please confirm

The DHT11 cannot be sampled faster than about once per second, and is polled every 2 s, while packets go out at 1 Hz. So most packets necessarily repeat the previous ambient reading.

The rule: **the last valid ambient reading is sent only while it is younger than 5 s; past that, `ambientTemperatureC` is `null`.**

This is the only place the firmware sends a value older than one packet interval, and it never applies to the skin reading — an invalid skin read is `null` immediately, always. Flagging it explicitly because "never send a prior reading as if it were live" is a contract rule and this is a bounded, deliberate exception to it. Say the word if you would rather have a shorter window, or an explicit age field (the latter would be a packet change).

### 3. Ambient resolution caveat

The DHT11 is coarse: **1 °C resolution, about ±2 °C accuracy, 0–50 °C range.** Please don't render it with decimals that imply more precision than it has, and don't difference it against skin temperature to produce anything that looks like a physics correction. Its job is spotting confounders — an HVAC cycle, device handling, contact loss, the sensor ending up in room air. The firmware does no correction with it and never will.

The DS18B20 by comparison is ±0.5 °C with 0.0625 °C steps, so `skinToAmbientC` is dominated by ambient error.

### 4. Exactly one packet per second — no event-driven extras

`docs/BLE_PROTOCOL.md` and `docs/sensor-protocol.md` say to also send a packet "whenever a value becomes unavailable or quality changes materially". The ECE lead context file says exactly one packet per second, and that takes precedence, so **no extra event-driven packets are sent**. Every change is reflected within 1 s regardless.

Practically this means the app can assume a fixed 1 Hz cadence and does not need to handle bursts. If you would rather have the event packets, that is a contract discussion — a variable rate would complicate the app's interval logic and the ML pipeline's resampling, so I would rather not.

### 5. Humidity is never transmitted

The DHT11 also measures humidity. It is logged to serial for debugging and is **not** in the packet, per the contract. `docs/experiment-plan.md` lists "ambient temperature and humidity" as an initial feature — if humidity is genuinely wanted for feature extraction, it needs a packet field, and therefore a contract change we both sign off on. It is not in there today.

### 6. Nothing here is verified against hardware yet

Everything above is verified in software: the build compiles clean, 47 native unit tests pass (including byte-exact packet checks and a scripted session simulation), and 20 host-side validator tests pass. **No part of it has run against physical sensors or a real phone yet** — that is the bench session in [test-plan-firmware.md](test-plan-firmware.md). I will send the three real packets and the stability note as soon as it is done.

---

## What I need from you

1. A decision on **time sync** (options above; I recommend B).
2. Confirmation that the **5 s ambient freshness rule** is acceptable.
3. Confirmation that **one packet per second with no event extras** works for the app.
4. A heads-up if the app needs anything else in the packet, so we can change the contract deliberately rather than discovering it on demo day.
