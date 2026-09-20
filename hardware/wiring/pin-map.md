# Krebb One pin map

Hardware documentation only. No firmware lives here; the firmware task reads the **Firmware pin contract** section below.

Krebb One is a research prototype. It is not a medical device, calorimeter, or calorie tracker. The DS18B20 reports a local **skin-contact temperature trend**; it does not measure calories or clinical body temperature. Only the change from the session baseline is meaningful.

Related files: [netlist-and-schematic.md](netlist-and-schematic.md) · [krebb-one-wiring.drawio](krebb-one-wiring.drawio) · [breadboard-build.md](breadboard-build.md) · [../bom/bom.md](../bom/bom.md) · [../test-plan-hardware.md](../test-plan-hardware.md)

## Sources and verification status

| Fact | Source | Status |
| --- | --- | --- |
| DevKitC-1 J1/J3 header pin order and GPIO numbers | [Espressif ESP32-S3-DevKitC-1 v1.1 user guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html), header tables (parsed from the raw page) | Verified against the guide. Not yet checked against your physical silkscreen. |
| Onboard RGB LED: GPIO48 on v1.0, GPIO38 on v1.1 | Espressif [v1.1 guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html) ("driven by GPIO38") and [v1.0 guide](https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.0.html) (GPIO48) | Verified against the guides |
| Strapping pins GPIO0, 3, 45, 46 and their defaults | [ESP32-S3 datasheet v2.2](https://www.espressif.com/sites/default/files/documentation/esp32-s3_datasheet_en.pdf), section 3 "Boot Configurations", Table 3-1 | Verified against the datasheet |
| GPIO26-32 flash/PSRAM; GPIO33-37 also reserved on octal-PSRAM chips; GPIO19/20 are USB-JTAG | [ESP-IDF GPIO API reference (ESP32-S3)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/api-reference/peripherals/gpio.html) and datasheet v2.2 pin-mux table (GPIO33-37 = octal SPI DQ4-DQ7/DQS) | Verified against the docs |
| DS18B20 pins, 3.0-5.5 V supply, 4.7 kOhm pull-up, ±0.5 °C (-10 to +85 °C), 750 ms max conversion at 12-bit | [Maxim/Analog DS18B20 datasheet](https://cdn-shop.adafruit.com/datasheets/DS18B20.pdf) (mirror; read from the PDF) | Verified against the datasheet |
| DHT11 pins VDD/DATA/NC/GND, 3-5.5 V supply, wait 1 s after power-up, ≥1 s between samples, 1 °C resolution, ±1 to ±2 °C | [Aosong DHT11 datasheet](https://www.adafruit.com/datasheets/DHT11-chinese.pdf) (Chinese-language original; read from the PDF) | Verified against the datasheet |
| Physical **left-to-right** order of the leads as seen from the front of each part | Convention / from memory (the datasheets give pin numbers and a bottom view, not a front-face photo) | **UNVERIFIED (check on the bench)**: see the pinout cards |
| Which end of the board is "pin 1" and USB-port placement | From memory of the Espressif pin-layout figure | **UNVERIFIED (check on the bench)**: go by your silkscreen |

## DevKitC-1 header reference (used to justify the pin choice)

From the Espressif v1.1 guide. J1 is the left header and J3 the right header when the antenna end is at the top and the USB ports are at the bottom (**UNVERIFIED (check on the bench)**: orientation; pin numbers are authoritative, direction is from memory).

```text
J1 (left)                          J3 (right)
 1  3V3                             1  G
 2  3V3                             2  TX  = GPIO43 (U0TXD)
 3  RST (EN)                        3  RX  = GPIO44 (U0RXD)
 4  GPIO4   <-- DS18B20 DQ          4  GPIO1
 5  GPIO5   <-- DHT11 DATA          5  GPIO2
 6  GPIO6                           6  GPIO42
 7  GPIO7                           7  GPIO41
 8  GPIO15                          8  GPIO40
 9  GPIO16                          9  GPIO39
10  GPIO17                         10  GPIO38  (RGB LED on v1.1)
11  GPIO18                         11  GPIO37  (octal PSRAM: avoid)
12  GPIO8                          12  GPIO36  (octal PSRAM: avoid)
13  GPIO3   (strapping)            13  GPIO35  (octal PSRAM: avoid)
14  GPIO46  (strapping)            14  GPIO0   (strapping, BOOT button)
15  GPIO9                          15  GPIO45  (strapping)
16  GPIO10                         16  GPIO48  (RGB LED on v1.0)
17  GPIO11                         17  GPIO47
18  GPIO12                         18  GPIO21
19  GPIO13                         19  GPIO20  (USB D+)
20  GPIO14                         20  GPIO19  (USB D-)
21  5V                             21  G
22  G                              22  G
```

GPIO26-34 are **not** broken out to the headers on this board, so they cannot be picked by accident. GPIO35-37 **are** on the headers (J3 pins 13/12/11) and must be avoided on octal-PSRAM modules.

## 1. Candidate GPIO analysis

The goal is two plain digital signals (DS18B20 1-Wire DQ, DHT11 single-wire DATA) that are safe on **every** ESP32-S3-WROOM-1 variant (quad or octal PSRAM), on **both** board revisions (RGB LED on GPIO48 or GPIO38), and that can never interfere with boot, flashing, USB, or the serial console.

| GPIO(s) | Header pin(s) | Verdict | Why |
| --- | --- | --- | --- |
| **0** | J3-14 | Unsafe | Strapping pin, weak pull-up, and the BOOT button. LOW at reset selects download boot mode (with GPIO46). A sensor line or its pull-up could interfere with boot/flashing. |
| **3** | J1-13 | Unsafe | Strapping pin (JTAG signal source), floating by default. Avoid. |
| **45** | J3-15 | Unsafe | Strapping pin (VDD_SPI voltage), weak pull-down. A wrong level at reset can misconfigure flash voltage. |
| **46** | J1-14 | Unsafe | Strapping pin (boot mode with GPIO0, ROM message printing), weak pull-down. |
| **19, 20** | J3-20, J3-19 | Unsafe | USB D- / D+ (USB-Serial-JTAG on the "USB" port). Repurposing them disables USB-JTAG/CDC. |
| **43, 44** | J3-2, J3-3 | Unsafe | UART0 TX/RX, wired to the on-board USB-to-UART bridge for the "UART" port. Used for flashing and the serial monitor. |
| **38** | J3-10 | Unsafe (board-dependent) | Onboard RGB LED on board **v1.1**. |
| **48** | J3-16 | Unsafe (board-dependent) | Onboard RGB LED on board **v1.0**. Also SPICLK_N. |
| **47** | J3-17 | Avoid | SPICLK_P (differential SPI clock alternate function). No benefit in using it; keep clear of the PSRAM/flash-adjacent group. |
| **26-32** | not on headers | Unsafe | SPI flash / PSRAM lines. Not broken out on DevKitC-1. |
| **33, 34** | not on headers | Unsafe on octal | Octal PSRAM DQ4/DQ5 on R8 chips. Not broken out. |
| **35, 36, 37** | J3-13, 12, 11 | Unsafe on octal | Octal PSRAM DQ6/DQ7/DQS on R8-type modules. Broken out to the headers, so a trap. |
| **39, 40, 41, 42** | J3-9, 8, 7, 6 | Avoid | Default JTAG pins (MTCK/MTDO/MTDI/MTMS). Usable as GPIO but noisy to debug if external JTAG is ever attached. |
| **RST (EN)** | J1-3 | Never | Chip reset. Not a GPIO. |
| **1, 2** | J3-4, J3-5 | Safe (spare) | ADC1/touch pins, no boot or module function. Held in reserve. |
| **4** | J1-4 | **Safe: chosen** | No strapping, USB, UART0, flash/PSRAM, JTAG or RGB function on any variant. ADC1_CH3/RTC/touch capable (unused here). Physically two pins from the 3V3 pin, which keeps wires short. |
| **5** | J1-5 | **Safe: chosen** | Same as GPIO4. Adjacent to GPIO4, so the two sensor wires leave from the same header area. |
| **6, 7** | J1-6, J1-7 | Safe (spare) | Same class as GPIO4/5. |
| **8-18, 21** | various | Safe (spare) | Generally usable; 15/16 double as the optional 32 kHz crystal pins and 8-14 double as SPI/touch, none used by this build. |

Other checks:

- **Input-only pins:** the ESP32-S3 has no input-only GPIOs (unlike the classic ESP32's GPIO34-39). The ESP-IDF GPIO page identifies none. Both signals need bidirectional pins (1-Wire and DHT11 are open-drain style buses), and GPIO4/5 are bidirectional.
- **Boot-sensitive concerns:** GPIO4 and GPIO5 are not sampled at reset. After reset they come up as high-impedance inputs (**from memory, verify against the datasheet IO MUX table**), so the external pull-ups hold each line HIGH during boot and neither sensor sees a spurious "start" pulse.
- **Logic level:** all ESP32-S3 GPIOs are 3.3 V and **not 5 V tolerant**. Both sensors are powered from 3V3 (never 5V), so DQ/DATA can never exceed 3.3 V.
- **Internal pull-ups:** do not rely on them. They are weak; both buses use **external** resistors.

## 2. Recommended defaults (kept)

| Signal | GPIO | Decision |
| --- | --- | --- |
| DS18B20 DQ | **GPIO4** | Kept. Verification found no problem. |
| DHT11 DATA | **GPIO5** | Kept. Verification found no problem. |

Power for both sensors comes from the DevKitC-1 **3V3** pin with a common **GND**. No sensor is powered from 5V or from a GPIO.

## 3. Final pin assignments

| Signal (net) | ESP32-S3 GPIO | DevKitC-1 header pin | Sensor pin | Resistor | Notes |
| --- | --- | --- | --- | --- | --- |
| 3V3 | (power) | J1 pin 1 "3V3" (J1 pin 2 is a second 3V3 pin, spare) | DS18B20 VDD; DHT11 VCC | none | Regulated 3.3 V from the on-board LDO. Powers both sensors. |
| GND | (ground) | J3 pin 1 "G" (any "G" pin works) | DS18B20 GND; DHT11 GND | none | J3-1 is chosen because, unlike J1-22, it is not next to the 5V pin (J1-21). |
| SKIN_DQ | **GPIO4** | J1 pin 4 "4" | DS18B20 DQ (TO-92 pin 2; probe yellow/white, verify) | **R1 = 4.7 kOhm from DQ to 3V3** | Powered mode (VDD to 3V3, not parasitic). |
| AMB_DATA | **GPIO5** | J1 pin 5 "5" | DHT11 DATA (bare pin 2; module "DATA/S/OUT", verify) | **R2 = 10 kOhm from DATA to 3V3, bare 4-pin part only.** None if the 3-pin module has one built in. | DHT11 also outputs humidity, which is **not** part of the BLE contract. |
| (none) | (none) | none | DHT11 NC (bare pin 3) | none | Leave unconnected. |

## Firmware pin contract

The firmware task reads this block. Names and numbers only. Do not change a number here without updating every hardware file.

```text
PIN_SKIN_ONEWIRE   = 4     # GPIO4, DS18B20 DQ, 1-Wire, external 4.7 kOhm pull-up to 3V3, sensor powered (not parasitic)
PIN_AMBIENT_DHT11  = 5     # GPIO5, DHT11 DATA, external pull-up (10 kOhm bare part / on-module), 3V3 powered
SENSOR_SUPPLY      = 3V3   # DevKitC-1 J1 pin 1; common GND on J3 pin 1

# Pins the firmware must NOT use for anything else (from the avoid list above)
PINS_RESERVED_STRAPPING = 0, 3, 45, 46
PINS_RESERVED_USB       = 19, 20
PINS_RESERVED_UART0     = 43, 44
PINS_RESERVED_RGB_LED   = 38, 48     # unknown board revision: avoid both, so no status LED is defined
PINS_RESERVED_PSRAM     = 26-37      # 26-32 always; 33-37 on octal-PSRAM variants
PINS_RESERVED_JTAG      = 39, 40, 41, 42, 47   # avoid; kept clear
```

Firmware notes recorded here because the hardware sets them (not solved in this task):

- DHT11 needs ≥1 s after power-up before the first read and ≥1 s between reads (datasheet); this document recommends treating **2 s** as the safe minimum. The BLE contract is 1 Hz and forbids reusing stale readings as live, so the firmware task must decide how to publish `ambientTemperatureC` between DHT11 samples (for example `null` when no fresh sample exists).
- DS18B20 12-bit conversion takes up to 750 ms, which fits inside a 1 s period only if the read is non-blocking or started early.
- The DHT11 is a coarse sensor (about 1 °C resolution, about ±2 °C accuracy): the ambient value is context for spotting confounders, not a precise measurement or a correction factor. See [../bom/bom.md](../bom/bom.md).

## 4. Component pinout cards

### DS18B20 (skin-contact temperature): powered mode

**Warning: reversing VDD and GND on a DS18B20 is widely reported to destroy it and make it heat up fast, and a hot part is a burn risk when it is taped to skin. Confirm the pinout before applying power, and never power a part that has a wire against skin until it has been checked on the bench.** (The datasheet lists absolute-maximum ratings but does not spell out this failure; treat it as a hard rule anyway.)

**Bare TO-92 package.** The datasheet gives pins numbered for the **BOTTOM VIEW** (looking at the lead ends): pin 1 = GND, pin 2 = DQ, pin 3 = VDD.

```text
   Flat face toward you, leads pointing down
   (UNVERIFIED (check on the bench): left/right mapping from the
    common convention; the datasheet is drawn from the BOTTOM view)

          .-----------.
         /  18B20      \      <- marking on the flat face
        |   (flat face) |
         \_____________/
            |   |   |
            |   |   |
           GND  DQ  VDD
          (1)  (2)  (3)
          left middle right
```

Before power: read the marking, look up your part's exact datasheet drawing, and match it to the continuity plan in [breadboard-build.md](breadboard-build.md). If the flat-face mapping and the datasheet's bottom view ever seem to disagree, trust the datasheet and the vendor's listing for your exact part, not this drawing.

**Waterproof probe (stainless tube with three flying leads).** Typical colors, **typical, verify with continuity/datasheet** (colors vary by seller):

| Wire | Typical function | Goes to |
| --- | --- | --- |
| Red | VDD | 3V3 |
| Black | GND | GND |
| Yellow or white | DQ | GPIO4 net (with R1 to 3V3) |

Before using a probe against skin, also check (**UNVERIFIED (check on the bench)**): with a multimeter in continuity mode, the stainless tube should be **open** to every one of the three leads. If the tube is connected to any lead, do not put it on skin without extra insulation.

Common facts (datasheet): supply 3.0-5.5 V, ±0.5 °C accuracy from -10 °C to +85 °C, up to 750 ms per 12-bit conversion, about 1 mA active current. Powered mode: VDD to 3V3, DQ pulled up with 4.7 kOhm. Parasitic mode (VDD tied to GND) is **not** used.

### DHT11 (ambient temperature; humidity is output but unused)

**Bare 4-pin part.** Datasheet pins: 1 VDD, 2 DATA, 3 NC, 4 GND. Front (grille/vent side) toward you, pins down, left to right (**UNVERIFIED (check on the bench)**: physical order from convention):

```text
          .-----------.
          |  .-----.  |
          |  | grid |  |   <- front (vented) face toward you
          |  '-----'  |
          '-----------'
            |  |  |  |
            |  |  |  |
           VCC DATA NC GND
           (1) (2) (3) (4)
```

Needs an external **10 kOhm** pull-up from DATA to 3V3 (the datasheet suggests about 5 kOhm for cables under 20 m; 10 kOhm is used here as specified, and 4.7 kOhm would also work; keep to one value and record it).

**3-pin module (PCB with the sensor and a pull-up resistor on board).** Pins: VCC, DATA, GND. **The order varies by manufacturer; read the silkscreen** (labels such as `+` / `OUT` / `-`, or `VCC` / `DATA` / `GND`, or `S` for signal). Do **not** add an external pull-up: the module already has one. Some modules also carry a power LED.

Common facts (datasheet): 3-5.5 V supply (so 3.3 V is in range), 0-50 °C, 1 °C resolution, about ±2 °C accuracy, 1 s wait after power-up, ≥1 s between samples, measuring current about 0.5-2.5 mA.

### Power notes

- Both parts run from 3.3 V (DS18B20 3.0-5.5 V, DHT11 3-5.5 V), so the DevKitC-1 3V3 pin can power both.
- Total added current is a few mA at most (DS18B20 about 1 mA active, DHT11 up to 2.5 mA measuring, plus about 0.7 mA through R1 and about 0.33 mA through R2 while a bus line is held low): small compared with the ESP32-S3 itself. Computed from datasheet figures; not measured.
- Power the board from a USB-C cable on a laptop or a battery power bank only. See [../placement-guide.md](../placement-guide.md).
- USB-C port choice: the board has two USB-C ports labeled **UART** (goes through the USB-to-UART bridge; the Espressif guide's recommended port for flashing and the serial monitor) and **USB** (the ESP32-S3's native USB, GPIO19/20). Default to the **UART** port for bring-up. Use one cable at a time. (The Espressif guide's text calls the UART connector Micro-USB; your board is described as USB-C. Go by what is physically on your board.)

## DECISIONS (this file)

1. **GPIO4 and GPIO5 kept** as the defaults; nothing found in verification contradicts them.
2. **GND on J3 pin 1**, not J1 pin 22, to avoid the adjacent 5V pin (J1 pin 21) when using jumper wires.
3. **Both RGB-LED pins (38 and 48) avoided** because the board revision is unknown, so no status LED is defined in the contract.
4. **Also avoided:** GPIO39-42 and 47 (JTAG/differential-clock functions) even though they are usable; costs nothing.
5. **External pull-ups on both lines**, never internal, so behavior does not depend on the firmware configuring anything before the buses go idle.
6. **DHT11 pull-up 10 kOhm, bare part only** as specified. The 3-pin module has its own, so R2 is omitted.
7. **UART port first** for bring-up because its serial path is independent of the ESP32-S3's native USB pins and firmware settings.
