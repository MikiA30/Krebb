# Krebb

Krebb is a personalized metabolic-response research prototype. It synchronizes contact temperature, ambient conditions, heart rate, and activity around a meal to characterize a person's physiological meal response. The hackathon prototype classifies response level; it does not present calorie intake as a validated measurement.

## System flow

```text
Krebb One sensors + Apple Watch / HealthKit
                ↓
       synchronized session data
                ↓
   feature extraction and local ML model
                ↓
       iOS response visualization
```

See [the system architecture](docs/architecture.md) and [the experiment plan](docs/experiment-plan.md) before collecting data.

## Repository layout

```text
apps/ios/                 SwiftUI app and tests
firmware/esp32/           Krebb One ESP32 firmware
firmware/arduino/         Arduino UNO Q alternative notes
ml/                       Local feature extraction and model work
shared/                   Cross-platform schemas and non-private sample data
hardware/                 Wiring, bill of materials, and CAD assets
scripts/                  Developer tooling, including sensor simulation
docs/                     Architecture, protocol, experiment, and demo plans
```

## HackMIT MVP

1. Collect a baseline and a post-meal sensor window.
2. Synchronize contact temperature, ambient conditions, heart rate, and activity.
3. Extract summary features and classify the observed response as no, small, moderate, or large.
4. Present the session and uncertainty clearly in the iOS app.

## Running components

The SwiftUI project will live in `apps/ios/` once created in Xcode. ESP32 firmware will live in `firmware/esp32/`. The ML pipeline will be runnable from `ml/` after its Python dependencies and scripts are added.
