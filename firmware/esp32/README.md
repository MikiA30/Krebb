# Krebb One ESP32 firmware

ESP32-S3 firmware for DS18B20 skin-contact temperature acquisition, DHT11 ambient-temperature acquisition, and BLE publishing. Milestones 1 (local sensor proof) and 2 (BLE contract proof).

Krebb One is a research prototype. It is **not a medical device, calorimeter, or validated calorie tracker**. The DS18B20 reports a local **skin-contact temperature trend** — not a clinical body temperature, and nothing about calories or metabolic rate. Only the within-session change from baseline carries meaning. Ambient temperature is confounder context (HVAC changes, device handling, contact loss, sensor exposure to room air), never a correction factor.

The firmware publishes measurements according to [the shared sensor protocol](../../docs/sensor-protocol.md) and [the BLE protocol](../../docs/BLE_PROTOCOL.md).

## Layout

```text
platformio.ini            build config; pinned platform and library versions
include/pins.h            pin contract, mirrors hardware/wiring/pin-map.md
lib/krebb_core/           PURE C++ (no Arduino headers), built for both envs
  src/krebb_params.h        every tunable, named and commented
  src/packet.{h,cpp}        JSON packet serialisation
  src/quality.{h,cpp}       sensorQuality heuristic
  src/freshness.{h,cpp}     ambient freshness rule
  src/time_source.{h,cpp}   Unix-ms clock with a dev-only fallback
src/main.cpp              scheduler, serial output, boot banner
src/skin_sensor.{h,cpp}   DS18B20 driver wrapper
src/ambient_sensor.{h,cpp} DHT11 driver wrapper
src/ble_service.{h,cpp}   NimBLE peripheral
scripts/                  build-time helpers (build epoch, pin-contract check)
test/test_native/         Unity suites, run on the host
tools/                    host-side BLE monitor and packet validator
```

## Hardware

| Signal | GPIO | Notes |
| --- | --- | --- |
| DS18B20 DQ | **4** | 1-Wire, external 4.7 kOhm pull-up to 3V3, **powered mode** (not parasitic) |
| DHT11 DATA | **5** | 3-pin module (sold as "CNT5"; it is a DHT11) with its own pull-up — no external resistor |
| 3V3 / GND | — | both sensors powered from the DevKitC-1 3V3 pin, common ground |

Board: ESP32-S3-DevKitC-1 with an ESP32-S3-WROOM-1-N8R8 module. PSRAM is **not** enabled, so GPIO35–37 stay unused. GPIO38/48 (RGB LED), the strapping pins, USB D±, and UART0 are all left alone — see [hardware/wiring/pin-map.md](../../hardware/wiring/pin-map.md).

`include/pins.h` must always match the "Firmware pin contract" block in the pin map. `scripts/check_pins.py` enforces it.

## Build, flash, monitor

PlatformIO is not assumed to be on your PATH. A virtualenv inside this directory works and is git-ignored:

```bash
cd firmware/esp32
python3 -m venv .venv
./.venv/bin/pip install platformio
```

Everything below uses `./.venv/bin/pio`; drop the prefix if you have `pio` globally.

```bash
# Build
./.venv/bin/pio run -e esp32s3

# Flash and then watch the serial output (port labelled UART)
./.venv/bin/pio run -e esp32s3 --target upload
./.venv/bin/pio device monitor -e esp32s3

# Both in one go
./.venv/bin/pio run -e esp32s3 --target upload --target monitor

# If the port is not auto-detected, list them and name one
./.venv/bin/pio device list
./.venv/bin/pio run -e esp32s3 --target upload --upload-port /dev/cu.usbserial-XXXX
```

Serial is **UART0 at 115200** through the on-board USB-to-UART bridge, i.e. the port labelled **UART**. `ARDUINO_USB_CDC_ON_BOOT` is deliberately not set.

**Alternative — the native USB port.** If you move the cable to the port labelled **USB**, uncomment the `[env:esp32s3_usbcdc]` block in `platformio.ini` and use `-e esp32s3_usbcdc`. Serial then runs over USB CDC, the port name becomes `/dev/cu.usbmodem*`, and the port disappears each time the chip reboots.

## Serial output

One status line per second, plus a `PKT` line carrying the exact packet:

```text
PKT {"timestampMs":1760000000000,"skinTemperatureC":33.42,"ambientTemperatureC":24.0,"sensorQuality":0.91}
[00123s] skin=33.42C ok | amb=24.0C age=1.2s hum=41% | q=0.91 STABLE win_range=0.06C consec=47s | ble=SUBSCRIBED mtu=247 | TIME=UNSYNCED(dev-fallback)
```

- `PKT` is printed every tick, whether or not anything is subscribed, so Milestone 1 can be checked without a phone. Whenever `ble=SUBSCRIBED`, the bytes after `PKT ` are byte-identical to what went out over BLE — that is what to copy for real example packets.
- `hum=` is a **serial-only** debug value. Humidity is not part of the BLE contract and is never transmitted.
- Failures print their own indented line before the status line, for example:

```text
  skin: DS18B20 disconnected (-127)
  skin: power-on-reset value 85.0 rejected
  amb: DHT11 checksum failed
  amb: last valid reading is 6.0s old (max 5.0s) - sending null
```

- `ble=` is `ADVERTISING`, `CONNECTED` (connected but not subscribed — nothing is sent), or `SUBSCRIBED` (active session).
- `TIME=` is `UNSYNCED(dev-fallback)` until the clock is set. See below.

The boot banner prints the firmware version, the build epoch, the pin map, the UUIDs, and the not-a-medical-device notice.

## Time and timestampMs

The contract requires `timestampMs` to be Unix milliseconds and never null, but this board has no RTC, no battery-backed clock, and no NTP, and **the shared protocol defines no time-sync mechanism**. Adding a writable time characteristic would be a contract change, so it is deliberately **not** done here — it is an open question for the software lead (see DECISIONS).

Until an epoch is supplied, the firmware uses `BUILD_UNIX_EPOCH_S * 1000 + millis()`, injected at build time by `scripts/inject_build_epoch.py`. That is only as correct as "when this firmware was compiled", so every status line says `TIME=UNSYNCED(dev-fallback)` until it is set.

To set it from the serial monitor (the only serial command):

```text
t 1760000000000
```

Get the value from your Mac:

```bash
python3 -c "import time; print(int(time.time()*1000))"
```

The line then reads `TIME=SYNCED`, and timestamps are real Unix milliseconds from that moment on.

## Verifying with nRF Connect (Milestone 2)

1. Flash the firmware and confirm the banner and 1 Hz status lines on serial.
2. Open **nRF Connect for Mobile** on the iPhone and tap **SCAN**.
3. Find **Krebb One** in the list. The advertisement carries the 128-bit service UUID; the name is in the scan response.
4. Tap **CONNECT**. The serial log prints `BLE: connected (handle N)` and then `BLE: MTU negotiated = ...`.
5. Expand the service **F000A001-0451-4000-B000-000000000000**.
6. Find characteristic **F000A002-0451-4000-B000-000000000000** (Notify).
7. Tap the **notify** icon (three downward arrows). Serial prints `BLE: client subscribed`, and the state becomes `ble=SUBSCRIBED`.
8. Set the value display to **UTF-8 text** (long-press the value, or use the value-format selector).
9. Confirm a full JSON object arrives about once per second, with all four fields and no truncation:
   `{"timestampMs":...,"skinTemperatureC":...,"ambientTemperatureC":...,"sensorQuality":...}`
10. Disconnect. Serial prints `BLE: disconnected - re-advertising` and the device reappears in a new scan.

## Native unit tests

Pure logic only — no hardware, under a second:

```bash
./.venv/bin/pio test -e native
```

Covers packet serialisation (including byte-exact contract examples), the quality heuristic, the freshness rule, the clock, and a scripted session simulation that walks through warm-up, stable contact, a contact-loss glitch, an unplugged sensor, recovery, and a DHT11 dropout.

## Host tools

```bash
python3 -m venv tools/.venv
tools/.venv/bin/pip install -r tools/requirements.txt

# Validator tests
tools/.venv/bin/pytest tools/ -q

# Live BLE monitor: find, connect, subscribe, validate, summarise on Ctrl+C
tools/.venv/bin/python tools/ble_monitor.py

# Record a session (JSONL + CSV under tools/recordings/, git-ignored)
tools/.venv/bin/python tools/ble_monitor.py --duration 120 --log-dir

# Reliability check: 5 connect/listen/disconnect cycles, pass/fail per cycle
tools/.venv/bin/python tools/ble_monitor.py --reconnect-test 5
```

On macOS the first run asks for Bluetooth permission for whichever app runs it (Terminal, iTerm, VS Code). If nothing is ever found, check **System Settings → Privacy & Security → Bluetooth**.

Check the pin contract at any time:

```bash
python3 scripts/check_pins.py
```

## Troubleshooting

**No serial port / `pio device list` shows nothing.** Confirm the cable is in the port labelled **UART**, not **USB**, and that it is a data cable rather than charge-only. Try another cable first — this is the most common cause. If you moved to the USB port, switch to the `esp32s3_usbcdc` env.

**Nothing on serial but the port exists.** Baud must be 115200. Press the RESET button to re-print the banner.

**`skin: DS18B20 disconnected (-127)`.** The 1-Wire bus reads all-ones: an open DQ line. Check the 4.7 kOhm pull-up from DQ to 3V3, that DQ really is on GPIO4, and that the sensor's GND and VDD are not swapped. On flexible skin leads this is usually a broken joint. The firmware re-scans the bus about once a second, so re-seating it recovers without a reboot.

**`skin: power-on-reset value 85.0 rejected`.** 85.0 exactly is the DS18B20 scratchpad default: the part browned out or was never converted. Usually a marginal supply or a long, thin 3V3 wire. It is rejected rather than reported, because it is not a measurement.

**`skin: conversion not complete at the read deadline`.** The 12-bit conversion did not finish inside its budget. If it persists, lower `CONVERSION_START_OFFSET_MS` or drop `SKIN_RESOLUTION_BITS` to 11 (375 ms) in `lib/krebb_core/src/krebb_params.h`.

**`amb: DHT11 checksum failed` / `timeout`.** Occasional failures are normal for a DHT11 and are handled — the last valid reading is used until it ages out. Persistent failures: check DATA is on GPIO5, that the module is on 3V3, and that the wire is short. The module has its own pull-up, so do **not** add an external one.

**`amb: last valid reading is N.Ns old - sending null`.** The DHT11 has not answered for more than 5 s, so the packet carries `null` instead of a stale number. Expected behaviour if the sensor is unplugged.

**`BLE ERROR: packet is N bytes but only M fit the negotiated MTU`.** The central negotiated a small MTU. The firmware refuses to send a truncated packet. nRF Connect can request a larger MTU manually; iOS negotiates a large one automatically.

**Phone cannot see the device.** Confirm serial shows `ble=ADVERTISING`. Force-quit and reopen the scanning app — iOS caches stale scan results aggressively. If it was connected before, the previous central may still hold the link; power-cycle the board.

**Packets stop but the phone is still connected.** Check for `ble=CONNECTED` rather than `SUBSCRIBED`: notifications were turned off. Re-enable notify.

## DECISIONS

1. **Exactly one packet per second; no event-driven extras.** `docs/BLE_PROTOCOL.md` and `docs/sensor-protocol.md` also ask for a packet "whenever a value becomes unavailable or quality changes materially". The ECE lead context file specifies one packet per second, and it takes precedence. Every change is visible within 1 s anyway, and a variable rate complicates the app's interval logic and the ML pipeline's resampling. Recorded as a discrepancy for the software lead.

2. **No time-sync characteristic.** Adding one would change the shared contract unilaterally. Instead: a build-time epoch fallback, a `t <unix_ms>` serial command, and an explicit `TIME=UNSYNCED(dev-fallback)` marker so a fallback timestamp is never mistaken for a real one. Escalated as an open question.

3. **Ambient freshness window of 5 s.** The DHT11 cannot be sampled faster than ~1 Hz and is polled every 2 s, while packets go out at 1 Hz, so most packets necessarily repeat the previous ambient reading. `AMBIENT_MAX_AGE_MS` bounds that at 5 s; past it the field is `null`. This is the **only** place an older-than-one-interval value is sent, and it never applies to the skin reading. Flagged for the software lead to confirm.

4. **12-bit DS18B20 with asynchronous conversion.** The conversion for tick N+1 starts 150 ms after tick N and is read at tick N+1: 150 + 750 = 900 ms, leaving 100 ms of margin. The value in each packet is measured within ~100 ms of being sent. 11-bit (375 ms) is the documented fallback if the bench shows pending-conversion errors.

5. **DHT11 reads confined to a phase window (300–700 ms after each tick).** A DHT11 read bit-bangs for ~25 ms with interrupts disabled. Keeping it away from packet assembly protects the 1 Hz notify jitter, and keeping it away from the conversion start protects the 1-Wire bus.

6. **Quality window discarded on any invalid read.** The consecutive-valid counter, the warmup, and the rolling window all reset. A window spanning a dropout would describe a contact that no longer exists, and the readings either side of a re-seat are not comparable.

7. **Locale-independent number formatting.** Values are rendered from scaled integers rather than `snprintf("%.2f")`, because `%f` honours the C locale's decimal separator; under a comma locale it would emit `33,42` and silently produce invalid JSON. It also makes the output byte-stable for the exact-string tests.

8. **`PKT` printed every tick, not only when transmitted.** Milestone 1 has no phone attached, and the line is still needed to see packets. The `ble=` field says whether it was actually sent; when `SUBSCRIBED`, the bytes are identical to the notification.

9. **NimBLE-Arduino pinned to 1.4.2.** 1.4.x and 2.x have incompatible callback signatures and type names. The code is written against the 1.4.x API, and the version is pinned exactly so a rebuild on demo day cannot pull a new major. Platform `espressif32@6.9.0` (Arduino core 2.0.17) is the matching known-good pair.

10. **Service UUID in the advertisement, name in the scan response.** A 128-bit UUID occupies 18 of the 31 advertising bytes, so the name would not reliably fit alongside it. This also lets `ble_monitor.py` find the device by UUID rather than by name.

11. **Callbacks only set atomic flags.** All logging and all notifying happen on the main loop, since NimBLE callbacks run on the host task. This is also why notifications are never sent from a callback.

12. **PSRAM left disabled.** The N8R8 module has 8 MB of octal PSRAM, but the workload fits in internal SRAM (9.3% used), and leaving it off keeps GPIO35–37 out of play entirely.

13. **`delay(1)` at the end of `loop()`.** Yields to the RTOS idle and BLE host tasks without affecting the millis()-based scheduler. The scheduler itself never uses a long delay.

14. **Build-time epoch injected by an extra script**, not hard-coded, so a fallback timestamp is at least near the flashing time rather than 1970.

15. **Sanity ranges are sensor bounds, not physiological claims.** Skin 5–50 °C and ambient 0–50 °C exist to catch wiring and contact faults; the DHT11 range is the datasheet's operating range. Neither says anything about a person.
