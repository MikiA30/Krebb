# Krebb One ESP32 firmware

This directory contains ESP32-S3 firmware for DS18B20 contact-temperature acquisition, ambient-temperature acquisition, and BLE publishing.

```
src/       Application entry points
include/   Shared headers
lib/       Local reusable libraries
test/      Firmware tests
```

The firmware publishes measurements according to [the shared sensor protocol](../../docs/sensor-protocol.md).
