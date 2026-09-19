# Architecture

## End-to-end flow

```text
TMP117 + BME280 + optional MAX30102        Apple Watch / HealthKit
                  │                                  │
                  └────────── BLE + iOS ─────────────┘
                                     ↓
                       synchronized measurement session
                                     ↓
                        feature extraction in Python
                                     ↓
                 lightweight response classifier / regression
                                     ↓
                        iOS session and trend visualization
```

## Responsibilities

- **ESP32 firmware:** acquires TMP117, BME280, and optional MAX30102 readings; publishes BLE packets.
- **iOS app:** guides baseline and post-meal sessions, receives BLE measurements, reads permitted HealthKit signals, and shows results.
- **ML pipeline:** joins synchronized signals, extracts session-level features, and produces a research response classification.
- **Shared layer:** defines packet and sample-session formats used by firmware, the simulator, the pipeline, and iOS.

## MVP inference

The MVP supports a response class: `no_meal`, `small`, `moderate`, or `large`. Any calorie range is an experimental research output and must show uncertainty.

## Backend decision

No backend is required for sensor collection, local ML experiments, or the initial demo. Add a serverless API only when the app needs a protected OpenAI API call or remote inference; never place an API key in the iOS app.
