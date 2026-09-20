#pragma once

// Krebb One pin contract.
//
// These numbers MUST equal the "Firmware pin contract" block in
// hardware/wiring/pin-map.md. scripts/check_pins.py parses both files and
// fails the build/verification if they ever drift apart.
//
// Deliberately plain constants with no Arduino headers, so host builds and
// native tests can include this file too.
//
// Pins NOT to use on this board (see pin-map.md for the full reasoning):
//   0, 3, 45, 46  strapping pins
//   19, 20        USB D-/D+
//   43, 44        UART0 (the port labelled UART, used for flashing + monitor)
//   38, 48        onboard RGB LED (v1.1 / v1.0 respectively)
//   26-37         SPI flash / PSRAM; 35-37 are octal-PSRAM lines on the N8R8

namespace krebb {

// DS18B20 1-Wire data (DQ), external 4.7 kOhm pull-up to 3V3, powered mode.
constexpr int PIN_SKIN_ONEWIRE = 4;

// DHT11 single-wire DATA. The 3-pin module carries its own pull-up, so there
// is no external resistor on this net.
constexpr int PIN_AMBIENT_DHT11 = 5;

}  // namespace krebb
