# Sensor protocol

Status: draft. Firmware and iOS owners must approve changes before implementation.

## Transport

Krebb One is an ESP32-S3 device that exposes a custom BLE service with a notify characteristic. Initial packets use UTF-8 JSON for rapid debugging. It reports DS18B20 contact temperature and ambient temperature. UUID details remain in [BLE_PROTOCOL.md](BLE_PROTOCOL.md).

## Packet

```json
{
  "timestampMs": 1760000000000,
  "skinTemperatureC": 33.4,
  "ambientTemperatureC": 24.8,
  "sensorQuality": 0.91
}
```

All readings except `timestampMs` may be `null` when unavailable. Each value must use the stated units. Never replace an unavailable value with a stale measurement.

## Update behavior

- Publish at 1 Hz during a session.
- Include a packet when availability or quality changes materially.
- iOS must tolerate unknown fields for forward compatibility.

HealthKit data belongs in a synchronized iOS session record; it is not transmitted by the device.
