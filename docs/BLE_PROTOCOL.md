# Krebb One BLE Protocol

This document is retained as the transport-level reference. The current product-level packet definition lives in [sensor-protocol.md](sensor-protocol.md).

Status: draft. Both the iOS and firmware owners must approve changes before implementation.

## Goals

- Stream current sensor measurements from Krebb One to the iOS app.
- Preserve units and missing-value behavior across both implementations.
- Keep the first hardware demo easy to inspect and debug.

## Initial transport

Krebb One exposes one custom BLE service with a notify characteristic for live measurements. For the first integration, the characteristic value is UTF-8 JSON. This favors reliable debugging during the hackathon; a compact binary format can replace it after the demo is stable.

## UUIDs

| Item | Value |
| --- | --- |
| Service UUID | `F000A001-0451-4000-B000-000000000000` |
| Measurement characteristic UUID | `F000A002-0451-4000-B000-000000000000` |

## Measurement packet

```json
{
  "timestampMs": 1760000000000,
  "skinTemperatureC": 33.4,
  "ambientTemperatureC": 24.8,
  "sensorQuality": 0.91
}
```

`skinTemperatureC` and `ambientTemperatureC` may be `null` when unavailable. `sensorQuality` is always a normalized number from `0.0` to `1.0`; it represents contact and measurement confidence, not medical certainty. Heart rate and activity are iOS/Apple Watch session context and are not emitted by the ESP32.

The ESP32 currently has no BLE time-sync characteristic. iOS records the phone arrival time for charts and session ordering, and preserves `timestampMs` as device-reported time for debugging and later reconciliation.

## Update behavior

- Publish at 1 Hz while the phone is subscribed to the measurement characteristic.
- The iOS app must tolerate unknown JSON fields for forward compatibility.
- Firmware must not reuse stale skin readings as current measurements.
- Ambient readings may repeat for a bounded freshness window because the DHT11 is sampled more slowly than 1 Hz; after that window, ambient is `null`.

## Ownership

- Firmware: advertising, connection handling, sensor acquisition, packet emission.
- iOS: scanning, connection handling, packet decoding, presentation, and mock-data fallback.
- Both: maintain this document when names, units, or behavior change.
