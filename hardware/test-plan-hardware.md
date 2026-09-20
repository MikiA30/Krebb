# Krebb One hardware bring-up test plan

A bench checklist you run yourself. **No firmware is required for anything here.** Procedures are in [wiring/breadboard-build.md](wiring/breadboard-build.md); pin numbers are in [wiring/pin-map.md](wiring/pin-map.md).

Krebb One is a research prototype. It is not a medical device. Do not put anything against skin until every box through section 5 is checked.

Record measured values in the blanks. Do not commit personal data. Nothing in this file needs to be committed with values filled in; keep your filled copy locally, or fill in the summary at the end and send it to the firmware task.

Tools: multimeter (continuity, resistance, DC volts), the built breadboard circuit, one USB-C cable, a laptop or battery power bank.

## 1. Identify the parts (do this first)

- [ ] Read the marking on the ESP32 module's metal shield. Write it down (for example `ESP32-S3-WROOM-1-N16R8`): ______________________
- [ ] Find the board revision on the PCB silkscreen (v1.0 or v1.1). If it is not printed, write "unknown": ______________
- [ ] Find the two USB-C ports. Confirm one is labeled **UART** and one **USB**.
- [ ] DS18B20 is: [ ] bare TO-92   [ ] waterproof probe   [ ] other: __________
- [ ] DHT11 is: [ ] bare 4-pin part   [ ] 3-pin module (pull-up on board)   [ ] other: __________
- [ ] Header pins on the ESP32 board are soldered on (needed for female-to-male jumpers).
- [ ] Silkscreen on the ESP32 board confirms the pin names used here: `3V3` at J1 pin 1, `4` and `5` on J1, `G` at J3 pin 1. (If your labels differ from [wiring/pin-map.md](wiring/pin-map.md), stop and reconcile before wiring.)

## 2. Sensor pinout confirmation (before any power)

- [ ] DS18B20 pinout checked against the datasheet drawing and, for a probe, wire colors confirmed. Pin used for GND: ______  DQ: ______  VDD: ______
- [ ] For a waterproof probe: stainless tube reads **open** to all three wires.
- [ ] DHT11 pinout checked against the datasheet (bare) or silkscreen (module). Pin used for VCC: ______  DATA: ______  GND: ______
- [ ] Resistors checked out of circuit: R1 reads about 4.7 kOhm (measured: ______ kOhm); R2 reads about 10 kOhm (measured: ______ kOhm, bare DHT11 only).

## 3. Visual inspection (ESP32 unplugged)

- [ ] Wiring matches the wire list in [wiring/breadboard-build.md](wiring/breadboard-build.md): W1 to W8, R1, and R2 (bare DHT11 only).
- [ ] No wire, lead, or resistor leg touches an adjacent column or rail.
- [ ] Stranded probe leads have no loose strands.
- [ ] The only header pins used are J1 pin 1, J1 pin 4, J1 pin 5, and J3 pin 1.
- [ ] Nothing is connected to the 5V pin.
- [ ] DHT11 sits away from the ESP32 board, in open air.

## 4. Continuity and resistance (ESP32 unplugged from USB)

| Check | Expect | Result |
| --- | --- | --- |
| [ ] DS18B20 VDD to J1 pin 1 (3V3) | continuity | ____ |
| [ ] DS18B20 GND to J3 pin 1 (G) | continuity | ____ |
| [ ] DS18B20 DQ to J1 pin 4 (GPIO4) | continuity | ____ |
| [ ] DHT11 VCC to J1 pin 1 (3V3) | continuity | ____ |
| [ ] DHT11 GND to J3 pin 1 (G) | continuity | ____ |
| [ ] DHT11 DATA to J1 pin 5 (GPIO5) | continuity | ____ |
| [ ] DHT11 NC (bare part) to anything | no continuity | ____ |
| [ ] J1 pin 4 (GPIO4) to J1 pin 5 (GPIO5) | no continuity | ____ |
| [ ] J1 pin 4 (GPIO4) to GND | no continuity | ____ |
| [ ] J1 pin 5 (GPIO5) to GND | no continuity | ____ |
| [ ] **J1 pin 1 (3V3) to J3 pin 1 (G): no short** | no beep, not near 0 ohm | ____ |
| [ ] 3V3 rail to GND rail: no short | no beep, not near 0 ohm | ____ |
| [ ] R1 (SKIN_DQ column to 3V3 rail) | about 4.7 kOhm (4.47 to 4.94 kOhm within 5%) | ____ kOhm |
| [ ] R2 (AMB_DATA column to 3V3 rail), bare DHT11 only | about 10 kOhm (9.5 to 10.5 kOhm within 5%) | ____ kOhm |
| [ ] Each rail beeps end to end (not split) | continuity | ____ |

## 5. First power (USB-C into the UART port, laptop or battery power bank)

- [ ] One cable only, into the port labeled **UART**. Power LED lights: [ ] yes  [ ] no
- [ ] J1 pin 1 (3V3) to J3 pin 1 (G) reads about 3.3 V (acceptance band 3.14 to 3.47 V): ______ V
- [ ] GPIO4 (J1 pin 4) to GND reads close to 3.3 V (pull-up): ______ V
- [ ] GPIO5 (J1 pin 5) to GND reads close to 3.3 V (pull-up): ______ V
- [ ] After about 30 s, DS18B20, DHT11, and both resistors are **not warm**.
- [ ] If a power bank is used: it stays on after a minute with this load.
- [ ] Unplug and re-plug once: the same readings repeat.

If any reading is wrong or anything is warm, unplug and go back to sections 2 to 4. Suspect a reversed DS18B20 first.

## 6. Ready for firmware gate

All must be true:

- [ ] Sections 1 to 5 are complete and every check passed.
- [ ] The 3V3 rail is about 3.3 V and nothing gets warm.
- [ ] Both sensor types and packages are recorded above.
- [ ] Module variant, board revision, and USB port are recorded below.
- [ ] Pin assignments in [wiring/pin-map.md](wiring/pin-map.md) still match the wiring exactly (GPIO4 = DS18B20 DQ, GPIO5 = DHT11 DATA).

Gate result: [ ] READY   [ ] NOT READY (reason: ____________________________)

## What the firmware task will need from me

Fill these in and hand them over with the pin map. Leave a field blank rather than guess.

| Item | Value |
| --- | --- |
| ESP32-S3 module variant (marking on the shield: N8, N8R2, N8R8, N16R8, ...) | __________________ |
| Flash size / PSRAM type, if known (quad or octal) | __________________ |
| Board revision (v1.0 / v1.1 / unknown) | __________________ |
| USB port used for flashing and serial (UART or USB) | __________________ |
| Serial device name that appears on the laptop (for example `/dev/cu.usbserial-*` or `/dev/cu.usbmodem*`) | __________________ |
| DHT11 type (bare 4-pin / 3-pin module) | __________________ |
| DHT11 pull-up fitted on the breadboard (R2 10 kOhm / module's own / other) | __________________ |
| DS18B20 type / package (bare TO-92 / waterproof probe / other) | __________________ |
| DS18B20 wire colors or lead order confirmed | __________________ |
| Power source used for bench tests (laptop / power bank) | __________________ |
| Date of the bench run and any anomalies | __________________ |

The firmware task reads the **Firmware pin contract** in [wiring/pin-map.md](wiring/pin-map.md): `PIN_SKIN_ONEWIRE = 4`, `PIN_AMBIENT_DHT11 = 5`.
