# Krebb

Krebb is a portable, multimodal health-screening prototype built around Krebb One, an ESP32-based sensing device and an iOS companion app.

## Repository layout

```text
ios/                     iOS application and tests
firmware/krebb-one/      ESP32 firmware for Krebb One
docs/                    Shared technical contracts and developer notes
tools/sensor-simulator/  Hardware-independent sample data and test tooling
```

Start cross-team implementation with [the BLE contract](docs/BLE_PROTOCOL.md). Keep it synchronized with both the iOS BLE client and the ESP32 firmware.
