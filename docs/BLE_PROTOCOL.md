# Krebb One BLE Protocol

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
  "heartRateBpm": 78,
  "spo2Percent": 97,
  "ambientTemperatureC": 24.8,
  "relativeHumidityPercent": 46.2,
  "vocPpb": 132,
  "breathSignal": 0.72,
  "quality": 0.91,
  "timestampMs": 1760000000000
}
```

All fields except `timestampMs` may be `null` when unavailable. `quality` is a normalized value from `0.0` to `1.0`; it represents measurement confidence, not medical certainty.

## Update behavior

- Publish at 1 Hz during an active measurement session.
- Send a packet whenever a value becomes unavailable or quality changes materially.
- The iOS app must tolerate unknown JSON fields for forward compatibility.
- Firmware must not reuse stale readings as current measurements.

## Ownership

- Firmware: advertising, connection handling, sensor acquisition, packet emission.
- iOS: scanning, connection handling, packet decoding, presentation, and mock-data fallback.
- Both: maintain this document when names, units, or behavior change.
