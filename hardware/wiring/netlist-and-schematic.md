# Krebb One netlist and schematic

Companion to [pin-map.md](pin-map.md), which is the source of truth for the pin numbers. The draw.io version is [krebb-one-wiring.drawio](krebb-one-wiring.drawio) (page 1 wiring, page 2 system block diagram).

Research prototype. Not a medical device. The DS18B20 gives a skin-contact temperature trend, not calories and not a clinical body temperature.

## Reference designators

| Ref | Part | Notes |
| --- | --- | --- |
| U1 | ESP32-S3-DevKitC-1 | Headers J1 (left) and J3 (right); pin numbers per Espressif guide |
| U2 | DS18B20 (bare TO-92 or waterproof probe) | Powered mode |
| U3 | DHT11 (bare 4-pin part or 3-pin module) | Ambient temperature (humidity unused) |
| R1 | 4.7 kOhm resistor | DQ pull-up to 3V3 |
| R2 | 10 kOhm resistor | DHT11 DATA pull-up to 3V3, **bare 4-pin part only** |

## Netlist

Format: `NET name` followed by every connection on it (`part.pin`).

```text
NET 3V3
  U1.J1-1        (DevKitC-1 pin "3V3"; J1-2 is a second 3V3 pin, left unused)
  U2.VDD         (TO-92 pin 3 / probe red, typical)
  U3.VCC         (bare pin 1 / module VCC)
  R1.1
  R2.1           (bare 4-pin DHT11 only)

NET GND
  U1.J3-1        (DevKitC-1 pin "G")
  U2.GND         (TO-92 pin 1 / probe black, typical)
  U3.GND         (bare pin 4 / module GND)

NET SKIN_DQ
  U1.J1-4        (GPIO4)
  U2.DQ          (TO-92 pin 2 / probe yellow or white, typical)
  R1.2

NET AMB_DATA
  U1.J1-5        (GPIO5)
  U3.DATA        (bare pin 2 / module DATA, S or OUT)
  R2.2           (bare 4-pin DHT11 only)

NO CONNECT
  U3.NC          (bare pin 3; leave unconnected)
```

Variant handling:

| Variant | What changes |
| --- | --- |
| DHT11 is a 3-pin module with a built-in pull-up | Delete R2 and the `R2.*` lines. Wire the module's three pins to 3V3, GND, and AMB_DATA per its silkscreen. |
| DHT11 is a bare 4-pin part | Keep R2 (10 kOhm) and leave pin 3 (NC) unconnected. |
| DS18B20 is a waterproof probe | Same three nets. Red to 3V3, black to GND, yellow/white to SKIN_DQ (typical, verify by continuity). |
| DS18B20 is a bare TO-92 | Same three nets. Pin 1 to GND, pin 2 to SKIN_DQ, pin 3 to 3V3. |

Net check: every GPIO appears once. GPIO4 exists only on SKIN_DQ and GPIO5 only on AMB_DATA. 3V3 and GND never share a part pin. Power is 3V3 only (no 5V in this design).

## ASCII schematic

Two rails, two signal lines. The `+` marks a connection; a `|` crossing a line without a `+` is not connected.

```text
3V3 (U1 J1-1)  ==========+==============+==============+==============+=====
                         |              |              |              |
                    U2.VDD(3)         [R1]        U3.VCC(1)         [R2]*
                                      4.7k                           10k
                                        |                             |
GPIO4 (J1-4) ---------------------------+--> U2.DQ(2)                 |
             [net SKIN_DQ]                                            |
GPIO5 (J1-5) ---------------------------------------------------------+--> U3.DATA(2)
             [net AMB_DATA]                     U3.NC(3): open

                         |                             |
                    U2.GND(1)                     U3.GND(4)
                         |                             |
GND (U1 J3-1)  ==========+=============================+====================

* R2 (10 kOhm) is fitted only for a bare 4-pin DHT11. A 3-pin module has its own pull-up.
  U3 = DHT11: VCC(1), DATA(2), NC(3), GND(4) for the bare part; VCC/DATA/GND per silkscreen for a module.
  U2 = DS18B20: GND(1), DQ(2), VDD(3) are TO-92 pin numbers.
```

Colors used in the draw.io file: **red** = 3V3, **black** = GND, **blue** = SKIN_DQ (GPIO4), **orange** = AMB_DATA (GPIO5).

## Consistency notes

- GPIO numbers appear only as 4 (SKIN_DQ) and 5 (AMB_DATA) in schematic, netlist, pin map, build sheet, and draw.io wiring page.
- None of the avoid-list pins (0, 3, 45, 46, 19, 20, 43, 44, 38, 48, 26-37) is connected. The draw.io page names them only in an "avoid" note.
- Header pin numbers: J1-1 3V3, J1-4 GPIO4, J1-5 GPIO5, J3-1 G. These match the Espressif header tables recorded in the pin map.
