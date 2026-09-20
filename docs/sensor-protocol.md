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

`skinTemperatureC` and `ambientTemperatureC` may be `null` when unavailable. `sensorQuality` is always a number in `[0.0, 1.0]`. Each value must use the stated units. Never replace an unavailable skin value with a stale measurement.

The current firmware does not have a BLE time-sync write. iOS uses phone arrival time for the session timeline and stores `timestampMs` as device-reported time.

## Update behavior

- Publish at 1 Hz while the phone is subscribed.
- iOS must tolerate unknown fields for forward compatibility.
- Ambient temperature may repeat within the firmware freshness window because the DHT11 is slower than the packet cadence; stale ambient becomes `null`.

HealthKit data belongs in a synchronized iOS session record; it is not transmitted by the device.
