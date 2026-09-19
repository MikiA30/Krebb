# Architecture

## End-to-end flow

```text
DS18B20 + ambient sensor                    Apple Watch / HealthKit
                  │                                  │
                  └──── ESP32-S3 -> BLE + iOS ───────┘
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

- **ESP32 firmware:** acquires DS18B20 contact-temperature and ambient-temperature readings; publishes BLE packets.
- **iOS app:** guides baseline and post-meal sessions, receives BLE measurements, reads permitted HealthKit signals, and shows results.
- **ML pipeline:** joins synchronized signals, extracts session-level features, and produces a research response classification.
- **Shared layer:** defines packet and sample-session formats used by firmware, the simulator, the pipeline, and iOS.

## MVP inference

The MVP supports a response class: `no_meal`, `small`, `moderate`, or `large`. Any calorie range is an experimental research output and must show uncertainty.

## Backend decision

No backend is required for sensor collection, local ML experiments, or the initial demo. Add a serverless API only when the app needs a protected OpenAI API call or remote inference; never place an API key in the iOS app.
