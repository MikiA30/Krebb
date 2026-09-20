# Krebb One firmware bring-up test plan

A bench checklist to run yourself, after the hardware checks in [test-plan-hardware.md](test-plan-hardware.md) have passed. Covers Milestone 1 (local sensor proof) and Milestone 2 (BLE contract proof).

Krebb One is a research prototype. It is **not a medical device**. The DS18B20 reports a skin-contact temperature trend, not a clinical body temperature and nothing about calories.

Build and flash commands, the serial format, and troubleshooting are in [firmware/esp32/README.md](../firmware/esp32/README.md). Everything below is **UNVERIFIED (needs hardware)** until you run it — no part of this plan has been executed against physical sensors.

Fill in the blanks as you go. Do not commit filled-in copies that contain participant data.

---

## 0. Before you start

- [ ] Hardware test plan passed: rails at ~3.3 V, continuity confirmed, nothing warm.
- [ ] USB-C cable in the port labelled **UART**.
- [ ] Toolchain ready:
  ```bash
  cd firmware/esp32
  python3 -m venv .venv && ./.venv/bin/pip install platformio
  ```
- [ ] Pin contract agrees: `python3 scripts/check_pins.py` prints `pin contract OK`.
- [ ] Native tests pass: `./.venv/bin/pio test -e native`.
- [ ] Builds clean: `./.venv/bin/pio run -e esp32s3`.

---

## 1. Milestone 1 — both sensors on serial

### 1.1 Boot

- [ ] `./.venv/bin/pio run -e esp32s3 --target upload --target monitor` completes.
- [ ] Boot banner shows the firmware version, build epoch, pin map (GPIO4 / GPIO5), UUIDs, and the not-a-medical-device notice.
- [ ] A status line appears about once per second.
- [ ] Observed loop cadence looks like 1 s: ______

### 1.2 DS18B20 (skin)

- [ ] `skin=` shows a plausible room-temperature value with the sensor untouched: ______ °C
- [ ] Pinching the sensor between two fingers makes the value rise within a few seconds.
- [ ] No `power-on-reset value 85.0` or `conversion not complete` errors during a 2-minute idle run. If either appears, note how often: ______
- [ ] `q=` starts in the 0.3–0.6 band and reaches ≥ 0.70 after about 60 s of steady contact. Time to reach 0.70: ______

### 1.3 DS18B20 unplug / replug

- [ ] Pull the DQ jumper. Within ~1 s, serial shows `skin: DS18B20 disconnected (-127)`.
- [ ] The `PKT` line shows `"skinTemperatureC":null` and `"sensorQuality":0.00`.
- [ ] **No `-127` anywhere in the `PKT` line.**
- [ ] The previous good value is **not** repeated in any packet while disconnected.
- [ ] `ambientTemperatureC` keeps updating — an ambient failure and a skin failure are independent.
- [ ] Reconnect the jumper. Readings return **without a reboot**, within about: ______ s
- [ ] `q=` restarts in the 0.3–0.6 band (warmup resets on a dropout), then climbs again.

### 1.4 DHT11 (ambient)

- [ ] `amb=` shows a plausible room temperature: ______ °C
- [ ] `age=` stays under 2.0 s in normal operation.
- [ ] `hum=` appears on the status line — and **never** inside the `PKT` line.
- [ ] Occasional `DHT11 checksum failed` lines are tolerated without the stream stopping. Failures seen in 5 minutes: ______

### 1.5 DHT11 unplug

- [ ] Pull the DHT11 DATA jumper.
- [ ] Failures are logged, and for the first ~5 s the packet still carries the last valid ambient value.
- [ ] After `AMBIENT_MAX_AGE_MS` (5 s), `ambientTemperatureC` becomes `null` and serial explains why.
- [ ] Measured time from unplug to the first `null`: ______ s
- [ ] The skin channel and `sensorQuality` are unaffected.
- [ ] Reconnect: ambient values return without a reboot.

### 1.6 Clock

- [ ] Every status line reads `TIME=UNSYNCED(dev-fallback)` before syncing.
- [ ] Get the time: `python3 -c "import time; print(int(time.time()*1000))"`
- [ ] Type `t <that number>` into the monitor and press Enter.
- [ ] Serial confirms `clock set: TIME=SYNCED`, and the status line now reads `TIME=SYNCED`.
- [ ] `timestampMs` in the `PKT` line matches wall-clock Unix ms, and increases by ~1000 each packet.

---

## 2. Milestone 2 — BLE contract proof

### 2.1 nRF Connect

Follow the numbered walkthrough in [firmware/esp32/README.md](../firmware/esp32/README.md).

- [ ] **Krebb One** appears in a scan.
- [ ] Connecting logs `BLE: connected` and `BLE: MTU negotiated = ____` on serial.
- [ ] Negotiated MTU: ______ (needs to be at least ~118; the packet is ~100–115 bytes)
- [ ] Service `F000A001-0451-4000-B000-000000000000` is present.
- [ ] Characteristic `F000A002-0451-4000-B000-000000000000` is present and Notify-capable.
- [ ] Enabling notify logs `BLE: client subscribed` and the status line shows `ble=SUBSCRIBED`.
- [ ] With the value shown as **UTF-8 text**, a complete JSON object arrives about once per second.
- [ ] All four fields are present, with the exact names and casing: `timestampMs`, `skinTemperatureC`, `ambientTemperatureC`, `sensorQuality`.
- [ ] **No humidity field and no extra fields.**
- [ ] Nothing is truncated — the object ends with `}`.
- [ ] No `BLE ERROR: ... MTU` lines on serial.
- [ ] Turning notify off stops the packets and logs `client unsubscribed`; nothing is sent while unsubscribed.

### 2.2 ble_monitor.py

```bash
cd firmware/esp32
python3 -m venv tools/.venv
tools/.venv/bin/pip install -r tools/requirements.txt
tools/.venv/bin/python tools/ble_monitor.py --duration 120 --log-dir
```

- [ ] The device is found **by service UUID** (not only by name).
- [ ] Packets print with an `ok` marker.
- [ ] Summary reports **100% valid**. Actual: ______ %
- [ ] Mean interval is close to 1000 ms. Actual: mean ______ / min ______ / max ______
- [ ] Jitter stays under about 50 ms.
- [ ] Null counts match what the sensors were doing.
- [ ] A JSONL and a CSV appear under `tools/recordings/` and are **not** picked up by `git status`.

### 2.3 Disconnect / reconnect

```bash
tools/.venv/bin/python tools/ble_monitor.py --reconnect-test 5
```

- [ ] 5/5 cycles pass.
- [ ] Serial shows `disconnected - re-advertising` after each cycle.
- [ ] No reboot is needed between cycles, and no cycle degrades relative to the first.
- [ ] Repeat once with nRF Connect (connect → subscribe → disconnect, 5 times): ______

### 2.4 Phone out of range

- [ ] With notify active, walk the phone out of range (or turn Bluetooth off).
- [ ] Serial logs the disconnect and returns to `ble=ADVERTISING`.
- [ ] Serial status lines and sensor sampling **continue** while nothing is connected.
- [ ] Return / re-enable Bluetooth: the device is discoverable again and a fresh subscription streams at 1 Hz.
- [ ] Time to become discoverable again: ______ s

---

## 3. Skin-stability procedure

The point is to record what a real baseline looks like, and to capture three genuine example packets for the software lead.

### 3.1 Setup

- [ ] Attach the DS18B20 at the **one** skin location chosen in [placement-guide.md](placement-guide.md) (a collarbone / upper-chest contact point is the candidate). Use the same location every session.
- [ ] Sensor fully insulated, held with medical-grade tape or a snug band, with strain relief on the lead.
- [ ] DHT11 in open room air, away from the body and away from the ESP32's heat.
- [ ] Power from the laptop or a battery power bank only.
- [ ] Record: location ____________, time since eating ____________, room conditions ____________, posture ____________.

### 3.2 Baseline run

- [ ] Set the clock first (`t <unix_ms>`) so the recording has real timestamps.
- [ ] Start recording: `tools/.venv/bin/python tools/ble_monitor.py --duration 900 --log-dir`
- [ ] Remain still for the full 10–15 minutes.
- [ ] Note any interruption with its timestamp: ____________________

### 3.3 What "stable" looks like

- [ ] `q=` reaches ≥ 0.70 and **stays** there: it should not oscillate back into the 0.3–0.6 band.
- [ ] `win_range` settles to a small value once contact is established. Observed: ______ °C
- [ ] `consec=` climbs continuously — a reset to 0 means contact was lost.
- [ ] Skin temperature rises after placement and then flattens. Time to flatten: ______ min
- [ ] Final baseline value: ______ °C (this is a contact trend, **not** a body temperature)
- [ ] Zero or near-zero null skin readings across the run. Nulls: ______

If quality never gets past 0.6, the usual causes are contact pressure that is too light, a sensor that keeps shifting, or a lead tugging the sensor off the skin. Re-tape and repeat rather than lowering the thresholds.

### 3.4 Capture three real example packets

- [ ] From the serial monitor, copy three **consecutive** `PKT` lines from the stable part of the run (`ble=SUBSCRIBED`, `TIME=SYNCED`, quality ≥ 0.70). Copy the text after `PKT `, exactly.
- [ ] Paste them into [handoff-to-software-lead.md](handoff-to-software-lead.md), replacing the `<<FILL WITH REAL DATA FROM MY HARDWARE RUN>>` marker.
- [ ] Also fill in the skin-stability note there.
- [ ] Do not include anything identifying a participant.

---

## 4. Ready-for-integration gate

- [ ] Sections 1 and 2 complete, every box ticked.
- [ ] A 15-minute run produced 100% valid packets with no unexplained gaps.
- [ ] Three real example packets captured and pasted into the handoff.
- [ ] Recordings are git-ignored and no raw participant data is staged.
- [ ] The software lead has the handoff document.

Gate result: [ ] READY for Milestone 3 (iOS integration)  [ ] NOT READY (reason: ____________________)
