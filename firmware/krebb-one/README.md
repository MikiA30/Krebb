# Krebb One firmware

This directory contains ESP32 firmware for sensor acquisition and BLE publishing.

```
src/       Application entry points
include/   Shared headers
lib/       Local reusable libraries
test/      Firmware tests
```

The firmware publishes measurements according to [the shared BLE contract](../../docs/BLE_PROTOCOL.md).
