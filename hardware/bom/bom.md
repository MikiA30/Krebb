# Krebb One bill of materials

Only parts you already have, plus the passives the circuit requires. Nothing here needs to be bought except possibly the two resistors and the jumper wires, if you do not already have them.

Krebb One is a research prototype. It is not a medical device, calorimeter, or calorie tracker. Wiring and pin numbers are in [../wiring/pin-map.md](../wiring/pin-map.md).

## Parts

| Qty | Part | Role | Status | Notes |
| --- | --- | --- | --- | --- |
| 1 | Espressif ESP32-S3-DevKitC-1 (ESP32-S3-WROOM-1 module) | Controller and BLE peripheral | Have | Module variant (N8, N8R2, N8R8, N16R8, ...) **unknown**; pins are chosen to be safe on all. Board revision (v1.0 / v1.1) unknown; both RGB-LED pins are avoided. Pin headers must be soldered on. |
| 1 | DS18B20 digital temperature sensor | Skin-contact temperature | Have | Package **unknown**: bare TO-92 or waterproof probe. Used in **powered mode** (not parasitic): VDD to 3V3. |
| 1 | DHT11 temperature/humidity sensor | Ambient (room) temperature | Have | Type **unknown**: bare 4-pin part or 3-pin module. Humidity is output by the part but is **not** part of the BLE contract. |
| 1 | USB-C cable (data-capable, not charge-only) | Power, flashing, serial | Have | Plug into the port labeled **UART** by default. See [../wiring/pin-map.md](../wiring/pin-map.md). |
| 1 | Solderless breadboard with power rails | Sensor circuit | Have | Half-size is enough. The ESP32 board stays beside it (see [../wiring/breadboard-build.md](../wiring/breadboard-build.md)). |
| 4 | Female-to-male jumper wires | ESP32 header to breadboard | Have (confirm) | 3V3, GND, GPIO4, GPIO5. |
| 4 (plus spares) | Male-to-male jumper wires | Rail and column links | Have (confirm) | |
| 1 | 4.7 kOhm resistor, 1/4 W | DS18B20 DQ pull-up to 3V3 (R1) | Required passive (confirm you have one) | Yellow, violet, red on a 4-band resistor. |
| 1 | 10 kOhm resistor, 1/4 W | DHT11 DATA pull-up to 3V3 (R2) | **Only for a bare 4-pin DHT11** | Brown, black, orange on a 4-band resistor. **Omit** for a 3-pin module that already has a pull-up. |
| 1 | Laptop USB port **or** battery power bank | Power source | Have | Never a non-isolated mains supply. See [../placement-guide.md](../placement-guide.md). |

Tool (not a BOM part): a multimeter with continuity/beep, resistance, and DC-volts modes is needed for the pre-power checks in [../test-plan-hardware.md](../test-plan-hardware.md).

Not part of the BOM, and not present in this build: thermal camera, TMP117, BME280, MAX30102, SpO2 sensor, VOC sensor, respiratory sensor, and any additional microcontroller. iPhone, Apple Watch, and MacBook are not wired to anything (system block diagram: [../wiring/krebb-one-wiring.drawio](../wiring/krebb-one-wiring.drawio), page 2).

## DHT11 limits (read before trusting the ambient number)

| Property | DHT11 | Source |
| --- | --- | --- |
| Temperature resolution | about 1 °C (8-bit integer) | Aosong datasheet |
| Temperature accuracy | about ±2 °C (datasheet lists ±1 typical to ±2 maximum) | Aosong datasheet; Adafruit product page |
| Range | 0 to 50 °C | Aosong datasheet |
| Sampling | no faster than about once every 1 s; treat **1 to 2 s** as the minimum interval, and 2 s as the safe choice | Aosong datasheet, Adafruit product page |
| Warm-up | wait about 1 s after power-up before the first read | Aosong datasheet |
| Humidity | about ±5 %RH accuracy; read by the part but **not** sent in the BLE packet | Aosong datasheet |

How to use the ambient value: it is **context for spotting confounders**, such as an HVAC change, the device being handled, skin-contact loss, or the sensor being exposed to room air. It is **not** a precise measurement and **not** a correction factor. Do not build a physics correction from it. With about 1 °C steps and about ±2 °C accuracy, only a change of a degree or more shows up at all; a slow drift smaller than that is invisible to it. Compare the ambient trend over a session, not its absolute value.

For scale, the DS18B20 datasheet gives ±0.5 °C accuracy from -10 to +85 °C and a 12-bit step of 0.0625 °C. Its within-session trend is the signal of interest; its absolute value is not a body-temperature claim.

## Decisions

1. A 10 kOhm resistor is listed **only** for a bare DHT11; a 3-pin module normally carries its own pull-up. Check by measuring, as described in the build sheet.
2. Female-to-male jumpers are listed because the ESP32 board stays off the breadboard (rationale in the build sheet).
3. No optional decoupling capacitor is listed. The DHT11 datasheet mentions an optional 100 nF between VDD and GND; it is not required for this build and is omitted so the BOM has no parts you do not already have. Add one only if the bench shows unstable DHT11 reads.
4. Skin-attachment materials (tape or band, heat-shrink) are covered in the placement guide and are not BOM items.
