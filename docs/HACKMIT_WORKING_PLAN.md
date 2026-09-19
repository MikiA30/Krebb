# HackMIT working plan

## Product decision

Krebb is a personalized metabolic-response research prototype. It records a quiet baseline and a post-meal observation window, then combines contact temperature, ambient context, and optional Apple Watch heart-rate/activity data into a preliminary response class:

- No observed meal response
- Small response
- Moderate response
- Large response

The demo must not claim to directly measure calories, perform calorimetry, diagnose a condition, or provide a validated nutritional result. The future vision is personal calibration that may improve meal tracking over time.

## Hardware that defines the MVP

| Available item | MVP role |
| --- | --- |
| ESP32-S3-WROOM | BLE sensor device and session streaming |
| DS18B20 | Contact-temperature time series |
| Ambient temperature sensor | Drift and environment context |
| iPhone | Session flow, BLE client, visualization, mock mode |
| Apple Watch | Optional heart-rate, activity, and steps context |
| MacBook | Firmware development, ML experiments, and demo fallback |

A thermal camera is not required. The DS18B20 must be held in a repeatable contact position with consistent pressure and insulation from room air as practical.

## Ambient temperature decision

Do not choose between ambient temperature and delta temperature; use both.

The pipeline should retain:

- `skinTemperatureC`
- `ambientTemperatureC`
- `deltaSkinTemperatureC = skinTemperatureC - baselineSkinTemperatureC`
- `skinToAmbientC = skinTemperatureC - ambientTemperatureC`
- ambient-temperature trend over the session

Ambient temperature is not a shortcut to an exact correction formula. It identifies confounding changes caused by room conditions, device handling, and sensor exposure. The model can use it as context; the app should flag an unstable environment or sensor-contact loss.

## Team operating model

### Product and iOS owner

- Own the SwiftUI app, BLE client, mock mode, session UX, HealthKit permissions, results screen, and final demo story.
- Keep `main` demoable and merge only tested work.
- Treat Apple Watch data as optional until it is verified in the app. The first complete flow must work with mock heart-rate data.

### Firmware and experiment owner

- Own DS18B20 and ambient-sensor wiring, ESP32-S3 firmware, BLE advertising, timestamped packets, and contact-stability checks.
- Own the physical placement guide and a short controlled data-collection protocol.
- Provide a recorded sensor session before the final demo, so integration does not depend on live hardware.

### Shared integration rules

- Agree on `docs/sensor-protocol.md` before firmware and iOS implementation diverge.
- Stream JSON at 1 Hz during a session for the initial prototype.
- Record timestamps, missing values, and quality flags; never substitute stale values for live readings.
- Keep raw participant data out of Git. Use synthetic sessions in `shared/sample-data/`.
- Work in short, focused branches such as `feature/ios-session-flow` and `feature/esp32-ble`. Merge a working checkpoint into `main` after each integration test.

## Optional external coding-tool track

Pursue an additional coding-tool sponsor track only if it produces one bounded, reviewable deliverable. Do not grant any external tool broad, ongoing repository access or let it modify the iOS app while active development is happening.

The best candidate is a reproducible Python feature-extraction script that accepts the shared session schema, handles missing Apple Watch data, produces the documented temperature and heart-rate features, and includes synthetic test data.

Keep tool-specific evidence and sponsor materials outside this public repository. The public commit history should remain attributable to the human team.

## Sponsor strategy

### Primary: Espressif

Krebb directly uses an ESP32-S3 as the sensing and BLE edge device. Demonstrate the full AIoT path:

```text
DS18B20 + ambient sensor -> ESP32-S3 -> BLE -> iPhone -> feature extraction -> response visualization
```

### Primary: OpenAI

OpenAI is a strong fit only if the product actually calls the OpenAI API. Use it for a meaningful explanation layer after the numerical classifier runs:

- Explain the observed response and data quality in plain language.
- Ask a focused follow-up when activity or contact quality makes the result unreliable.
- Clearly distinguish measured facts from the experimental response classification.

Do not send an OpenAI API key in the iOS app. Add a small protected serverless endpoint only after the sensor-to-app flow works. Prepare any sponsor-specific development-process evidence outside the public repository.

### Conditional: coding-tool sponsor track

Submit only if an external coding tool completes the isolated feature-extraction task or another equally meaningful, reviewable deliverable. Keep screenshots or a session link as private sponsor evidence.

### Conditional: Voloridge

Submit only after selecting a relevant dataset from Voloridge's curated public-data collection. The team's own sensor traces alone do not satisfy the requirement. A valid approach would combine the public dataset with Krebb's local experiment to explain what is known, what is personalized, and what remains uncertain.

### Do not prioritize: Arduino

The Arduino challenge explicitly requires an Arduino UNO Q. Do not split the core sensing effort away from ESP32-S3 unless the team obtains and commits to the UNO Q path.

### Do not prioritize: Meta, Deepgram, and other adjacent tracks

They would add product surface area without strengthening the core sensing, data, and demo loop. Reconsider them only after the end-to-end MVP works.

## Build sequence

### 1. Establish the data contract

Update the protocol and schema for the actual hardware:

```json
{
  "timestampMs": 1760000000000,
  "skinTemperatureC": 33.4,
  "ambientTemperatureC": 24.8,
  "sensorQuality": 0.91
}
```

Heart rate, steps, and activity are iOS/HealthKit context rather than ESP32 values.

### 2. Prove hardware-to-phone transport

Display live contact and ambient temperature in an iOS mock-compatible session. Add a visible quality state: stabilizing, ready, contact lost, or unavailable.

### 3. Collect one controlled experiment

Record a 10-15 minute still baseline, followed by a labeled no-meal or meal observation window. Note movement, caffeine, room change, and interruptions. Do not rely on an unrecorded live meal response during judging.

### 4. Build the data loop

Save a synchronized session, extract features, and classify it. With limited data, show raw trends plus an experimental class instead of claiming statistical validation.

### 5. Add explanation and polish

Only after steps 1-4 work, add the OpenAI explanation layer and prepare private sponsor-demo evidence.

## Definition of demo-ready

- ESP32-S3 streams DS18B20 and ambient readings to the iPhone.
- The app completes a baseline and displays measurement quality.
- The app loads or records a synchronized post-meal session.
- The ML pipeline extracts features and returns an experimental response class.
- A pre-recorded successful session is available as a fallback.
- The demo explains limitations honestly and shows the future personalized-calibration vision.
