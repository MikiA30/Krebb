# Breadboard build sheet

Build the Krebb One sensor circuit on a solderless breadboard, wire by wire, then check it with a multimeter **before** the board is powered. Pin numbers come from [pin-map.md](pin-map.md); nets from [netlist-and-schematic.md](netlist-and-schematic.md). No firmware is needed for anything on this page.

Research prototype. Not a medical device. Do not put any part against skin until it has passed the pre-power checklist below and the first-power check.

## Design choice: the ESP32 stays off the breadboard

The DevKitC-1's two header rows are about 1 inch apart, which leaves **at most one free hole column** beside them when the board straddles a standard breadboard (**UNVERIFIED (check on the bench)**: from memory of the board dimensions, not measured on your parts). That is not enough to wire from. So this build keeps the ESP32-S3 board **beside** the breadboard and connects it with four **female-to-male jumper wires** (female end on the ESP32 header pin, male end in the breadboard). That layout works with any breadboard width and makes every header connection easy to unplug and verify.

If your DevKitC-1 shipped with the pin headers **unsoldered**, solder them first. Female-to-male jumpers need male pins on the board.

## Parts for this build

See [../bom/bom.md](../bom/bom.md). For this sheet: breadboard with a power-rail pair (half-size is enough), 4 female-to-male jumpers, 4 male-to-male jumpers (plus spares), 1 x 4.7 kOhm, 1 x 10 kOhm (bare DHT11 only).

## Breadboard conventions used here

- The breadboard's two long power rails are used as the **3V3 rail (+, red)** and the **GND rail (-, blue/black)**.
- Some full-size breadboards have rails that are split in the middle. Check with the multimeter in continuity mode (end to end along each rail) before trusting a rail.
- A numbered column is connected across its **five holes on one side of the center channel**. This build uses only the five holes on the rail side, called **row 1** (nearest the rails) to **row 5** (nearest the center channel). The two sides of the center channel are **not** connected.
- Column numbers below are only labels for this sheet; you can shift the whole layout left or right.

## Target layout (rows and columns)

Columns 5-7 hold the DS18B20 and columns 10-13 hold the DHT11. The rails run along the top edge.

```text
 + rail (3V3)  ==========================================================  <- W1 from ESP32 J1-1
 - rail (GND)  ==========================================================  <- W2 from ESP32 J3-1

 column        5      6      7      8   9    10     11     12     13
               GND    DQ     VDD              VCC    DATA   NC     GND
               (DS18B20)                       (DHT11, bare 4-pin)

 row 1         W3     R1     W4               W6     R2*           W7
                      |                              |
                   (R1 other                      (R2 other
                    lead -> + rail)                lead -> + rail)
 row 2                W5                              W8
                    (from                          (from
                     GPIO4)                         GPIO5)
 row 3         GND    DQ     VDD              VCC    DATA   NC     GND
               pin    pin    pin              pin    pin    pin    pin     <- the sensor pins go here
 row 4
 row 5

 * R2 only for a bare 4-pin DHT11.
```

What each occupied hole means:

| Column | Row 1 | Row 2 | Row 3 |
| --- | --- | --- | --- |
| 5 (DS18B20 GND) | W3 black jumper to the GND rail | | DS18B20 GND lead (TO-92 pin 1 / probe black) |
| 6 (SKIN_DQ) | R1 (4.7 kOhm), other lead into the 3V3 rail | W5 blue jumper to ESP32 GPIO4 (J1-4) | DS18B20 DQ lead (TO-92 pin 2 / probe yellow or white) |
| 7 (DS18B20 VDD) | W4 red jumper to the 3V3 rail | | DS18B20 VDD lead (TO-92 pin 3 / probe red) |
| 10 (DHT11 VCC) | W6 red jumper to the 3V3 rail | | DHT11 VCC (bare pin 1) |
| 11 (AMB_DATA) | R2 (10 kOhm), other lead into the 3V3 rail (bare part only) | W8 orange jumper to ESP32 GPIO5 (J1-5) | DHT11 DATA (bare pin 2) |
| 12 | | | DHT11 NC (bare pin 3): nothing else on this column |
| 13 (DHT11 GND) | W7 black jumper to the GND rail | | DHT11 GND (bare pin 4) |

The sensors' leads are all in **row 3** of their columns. Each column is a separate net, so no two leads of one sensor touch.

## Wire list

| Wire | Type | From | To | Net |
| --- | --- | --- | --- | --- |
| W1 (red) | female-to-male | ESP32 J1 pin 1 (3V3) | + rail | 3V3 |
| W2 (black) | female-to-male | ESP32 J3 pin 1 (G) | - rail | GND |
| W3 (black) | male-to-male | column 5, row 1 | - rail | GND |
| W4 (red) | male-to-male | column 7, row 1 | + rail | 3V3 |
| R1 | 4.7 kOhm resistor | column 6, row 1 | + rail | SKIN_DQ to 3V3 |
| W5 (blue) | female-to-male | ESP32 J1 pin 4 (GPIO4) | column 6, row 2 | SKIN_DQ |
| W6 (red) | male-to-male | column 10, row 1 | + rail | 3V3 |
| W7 (black) | male-to-male | column 13, row 1 | - rail | GND |
| R2 | 10 kOhm resistor, **bare part only** | column 11, row 1 | + rail | AMB_DATA to 3V3 |
| W8 (orange) | female-to-male | ESP32 J1 pin 5 (GPIO5) | column 11, row 2 | AMB_DATA |

Wire colors are a convention to keep the build readable; the electrical meaning is in the "Net" column, so use whatever jumper colors you have and label them.

## Build order

Do all of this with the ESP32 board **unplugged from USB**.

1. **Identify your parts first.** Read [pin-map.md](pin-map.md) and note: DS18B20 is a bare TO-92 or a waterproof probe; DHT11 is a bare 4-pin part or a 3-pin module. Write the answers in [../test-plan-hardware.md](../test-plan-hardware.md).
2. **Confirm the DS18B20 pinout before it goes anywhere near power.** Bare TO-92: datasheet pin 1 GND, 2 DQ, 3 VDD (drawn from the bottom view); see the card in [pin-map.md](pin-map.md). Probe: typical colors are red VDD, black GND, yellow/white DQ, but **verify, do not assume**. Reversing VDD and GND on a DS18B20 can destroy it and make it heat up fast.
3. **Confirm the DHT11 pinout.** Bare part: VCC, DATA, NC, GND (datasheet pins 1-4). Module: read the silkscreen, because the order of VCC/DATA/GND varies.
4. **Place the resistors first.** R1 (4.7 kOhm: yellow, violet, red bands on a 4-band resistor) from column 6 row 1 to the + rail. If bare DHT11: R2 (10 kOhm: brown, black, orange bands) from column 11 row 1 to the + rail. Confirm both values on the multimeter (resistance mode) before inserting them.
5. **Rail jumpers.** W3 (col 5 row 1 to - rail), W4 (col 7 row 1 to + rail), W6 (col 10 row 1 to + rail), W7 (col 13 row 1 to - rail).
6. **Seat the DS18B20 in row 3 of columns 5, 6, 7.**
   - Bare TO-92: flat face toward you, leads down into the board. Left GND (col 5), middle DQ (col 6), right VDD (col 7). This left/right mapping is from convention and the datasheet's bottom view, **UNVERIFIED (check on the bench)**, so verify with the continuity check below before power.
   - Waterproof probe: black to col 5, yellow/white to col 6, red to col 7. Twist stranded ends tight, or crimp/screw on a pin, so no strand can touch a neighbor.
7. **Seat the DHT11 in row 3 of columns 10-13.**
   - Bare 4-pin part: front (grille) face toward you, pins down: VCC col 10, DATA col 11, NC col 12, GND col 13.
   - 3-pin module: use columns 10, 11, 12 in the module's own pin order, then move the row-1 jumpers so that the column holding **VCC** goes to the 3V3 rail, the column holding **GND** goes to the GND rail, and the column holding **DATA** gets the GPIO5 wire (W8). Wire by label, not by column position. Omit R2.
8. **Signal wires last.** W5 (blue) from ESP32 J1 pin 4 to col 6 row 2. W8 (orange) from ESP32 J1 pin 5 to col 11 row 2.
9. **Power wires.** W1 (red) from ESP32 J1 pin 1 to the + rail. W2 (black) from ESP32 J3 pin 1 to the - rail. Make these the last two connections.
10. **Physical placement.** Keep the DHT11 as far from the ESP32 board (a heat source) as the jumpers allow, in open air. See [../placement-guide.md](../placement-guide.md).
11. **Do not connect USB yet.** Go to the pre-power checklist.

## Pre-power checklist (multimeter, ESP32 unplugged from USB)

Use continuity/beep mode unless a resistance value is asked for. Touch the probe to the metal of the female jumper end or the header pin. Record results in [../test-plan-hardware.md](../test-plan-hardware.md).

- [ ] **Visual.** No bare wire ends or resistor leads touching a neighbor; no resistor lead crossing another column; the DS18B20 and DHT11 leads are only in the intended columns.
- [ ] **Sensor pinout orientation confirmed** against the datasheet drawing (DS18B20) and the silkscreen or datasheet (DHT11) with the part in hand. For a waterproof probe, colors confirmed by continuity.
- [ ] **Continuity, each sensor pin to the correct header pin:**

| From | To | Expect |
| --- | --- | --- |
| DS18B20 VDD (col 7 row 3) | ESP32 J1 pin 1 (3V3) | continuity |
| DS18B20 GND (col 5 row 3) | ESP32 J3 pin 1 (G) | continuity |
| DS18B20 DQ (col 6 row 3) | ESP32 J1 pin 4 (GPIO4) | continuity |
| DHT11 VCC (col 10 row 3, or module VCC) | ESP32 J1 pin 1 (3V3) | continuity |
| DHT11 GND (col 13 row 3, or module GND) | ESP32 J3 pin 1 (G) | continuity |
| DHT11 DATA (col 11 row 3, or module DATA) | ESP32 J1 pin 5 (GPIO5) | continuity |
| DHT11 NC (col 12 row 3, bare part) | any other pin | **no** continuity |
| DS18B20 DQ | DS18B20 VDD or GND | **no beep** (separate nets; DQ to VDD reads a resistance of about R1 through the 3V3 rail, and DQ to GND reads open or high) |

- [ ] **No 3V3-to-GND short.** With the ESP32 unpowered, J1 pin 1 to J3 pin 1: must **not** beep or read near 0 ohm. Also check the + rail against the - rail. (An exact expected resistance is not stated because it depends on the board: **UNVERIFIED (check on the bench)**. A low reading that climbs while you watch is charging capacitors; a steady near-zero reading is a short.)
- [ ] **Pull-up R1 reads about 4.7 kOhm** between column 6 (SKIN_DQ) and the + rail. Within ±5% is 4.47 to 4.94 kOhm (arithmetic on the tolerance, not a measurement).
- [ ] **Pull-up R2 reads about 10 kOhm** between column 11 (AMB_DATA) and the + rail, **bare DHT11 only**. Within ±5% is 9.5 to 10.5 kOhm. With a 3-pin module, this column should instead read a module-dependent value or open; if you also read about 10 kOhm on the module, note that the module's own pull-up value in the test plan and do not add R2.
- [ ] **Rail integrity.** Each rail beeps end to end and the + and - rails do not beep to each other.
- [ ] **Waterproof probe only:** the stainless tube reads **open** to each of the three leads. If it does not, do not use that probe against skin without extra insulation.

## First-power check

1. Plug a USB-C cable into the **UART**-labeled port (default; see [pin-map.md](pin-map.md)) of the ESP32 board, from the laptop or a battery power bank. Use one cable only.
2. Note whether the board's power LED lights.
3. Multimeter, DC volts: J1 pin 1 (3V3) to J3 pin 1 (G) reads about 3.3 V. As a bench acceptance band use 3.3 V ±5% (3.14 to 3.47 V). This is the acceptance band used here, not an Espressif specification.
4. GPIO4 and GPIO5 to GND should each read close to 3.3 V, because the pull-ups hold them high (**UNVERIFIED (check on the bench)**: whatever the board's factory or leftover firmware does may drive them).
5. After about 30 seconds, feel the DS18B20, the DHT11 and the resistors with a fingertip. **Nothing should feel warm.** The ESP32 module itself may be slightly warm.
6. If anything is warm, hot, smells, or a voltage is wrong: unplug USB immediately and go back to the continuity checks. Suspect a reversed DS18B20 first.
7. If all is well, the board is at the "ready for firmware" gate in [../test-plan-hardware.md](../test-plan-hardware.md).

## Notes

- Do not power the sensors from 5V. The ESP32-S3 GPIOs are 3.3 V only and are not 5V tolerant; a 5V pull-up on either data line could damage the chip.
- Power-bank caveat: some power banks turn off when the load current is small. Whether yours stays on is **UNVERIFIED (check on the bench)**. If it switches off, use the laptop for bench work.
- The DHT11 needs about 1 s after power-up before it responds (datasheet). That matters to the firmware task, not to this build.
