# Krebb

Krebb is a hardware and iPhone prototype for effortless meal-response tracking. Instead of asking people to weigh every ingredient, scan every label, or guess restaurant calories, Krebb measures how the body responds after eating and turns that signal into a personal food-response profile.

The project was built at HackMIT 2026 by a chemical engineering and computer science student from Cornell and an electrical engineering student from MIT. The idea came from a personal weight-loss journey: after losing 55 pounds, it became clear that the science of weight management is deeply tied to thermodynamics, metabolism, heat, behavior, and consistency. The hard part is not knowing that energy balance matters. The hard part is making healthy tracking convenient enough that people can actually keep doing it.

## The problem

America has a major metabolic-health problem. Diet and weight are tied to some of the largest causes of death and chronic disease, including heart disease and diabetes. Many products try to help by tracking what goes into the body: food weight, portion size, barcode scans, restaurant estimates, photos, or manual calorie logs.

Those approaches can work, but they are inconvenient. They break down when people eat at restaurants, share food, forget to log, do not know the recipe, or do not want to weigh everything they eat. Lab-grade metabolic testing can measure energy expenditure more directly, but it is expensive, inconvenient, and not something normal people can use every day.

Krebb takes a different approach. We looked at the problem from a chemical-engineering and thermodynamics perspective: after food enters the body, digestion and metabolism create measurable physiological changes. Krebb explores whether a low-cost personal sensor system can capture part of that response and use it for practical, personalized meal tracking.

## What Krebb does

Krebb records a short baseline before a meal, then tracks the user's response after the first bite. The prototype combines:

- an ESP32-S3 WROOM microcontroller,
- a DS18B20 contact temperature probe,
- ambient temperature context,
- Bluetooth Low Energy streaming,
- an iPhone SwiftUI app,
- optional Apple Watch / Apple Health context,
- a transparent response classifier,
- and an OpenAI-powered coach that explains results in plain language.

The current prototype does not claim to be a validated calorie estimator. It classifies early meal response and saves labeled calibration examples. With enough meals from the same person, the same feature pipeline can become a personalized model that estimates meal impact more conveniently than manual logging alone.

## Demo results

During the hackathon, we collected real hardware sessions from the iPhone app:

| Trial | Known calories | Result |
|---|---:|---|
| No-meal control | none | Mostly flat response, useful as a baseline comparison |
| Donut | 240 | Clear post-meal response |
| Turkey sausage, egg, and cheese sandwich | about 450 | Stronger early response than the donut |

The sandwich trial produced a strong early response with hundreds of live sensor samples. Krebb Coach then translated the raw session features into consumer-friendly feedback, explaining what changed, what to do next, and what the system cannot claim yet.

## How it works

```text
Contact temperature + ambient sensor
                ↓
ESP32-S3 firmware over BLE
                ↓
iPhone SwiftUI app
                ↓
Baseline + post-meal session recording
                ↓
Feature extraction
                ↓
Response classification + AI explanation
                ↓
Personal calibration history
```

A session has two phases:

1. **Baseline** - The user holds still while Krebb records a starting point.
2. **Observation** - The user marks the first bite, then Krebb records the post-meal response.

The app stores sessions locally, displays the response curve, extracts features, and lets the user delete unwanted journal entries. For demos and research, the app can export a JSON session package for analysis.

## Why this is different

Most consumer nutrition tools begin with the food and ask, "What did you eat, and how much?"

Krebb begins with the body and asks, "How did you respond?"

That does not replace nutrition labels or calorie science. It adds a more personal layer for situations where intake is hard to know, especially restaurant meals, mixed foods, and real life outside a kitchen scale.

The long-term goal is a system that learns a user's personal response patterns over time. A 450-calorie meal does not affect every person in exactly the same way, and even the same person can respond differently depending on context, timing, activity, sleep, stress, and health. Krebb is designed around that personal calibration problem.

## OpenAI-powered coach

Raw sensor features are not useful to most people. Krebb uses the OpenAI API to turn a recorded session into a short, plain-language explanation:

```text
1) Your sandwich caused a strong early response after eating.
2) Save a few similar meals so Krebb can learn your usual pattern.
3) This is an early signal, not a calorie estimate yet.
```

The coach is deliberately constrained. It does not give medical advice and does not claim validated calorie estimation. Its job is to make the sensor result understandable and actionable.

## Repository layout

```text
apps/ios/                 SwiftUI iPhone app and tests
firmware/esp32/           ESP32-S3 firmware for Krebb One
hardware/                 Wiring, bill of materials, and build notes
ml/                       Feature extraction, classifier, and OpenAI coach script
shared/                   Schemas and sample-data structure
docs/                     Architecture, BLE protocol, experiment, and demo notes
scripts/                  Developer tooling and simulator notes
```

Important files:

- [`apps/ios/README.md`](apps/ios/README.md) - iOS app notes
- [`docs/BLE_PROTOCOL.md`](docs/BLE_PROTOCOL.md) - Bluetooth packet contract
- [`docs/hackmit-demo.md`](docs/hackmit-demo.md) - demo flow and sponsor framing
- [`ml/scripts/classify_session.py`](ml/scripts/classify_session.py) - transparent response classifier
- [`ml/scripts/openai_coach.py`](ml/scripts/openai_coach.py) - OpenAI-powered explanation script

## Technology stack

**Hardware**

- ESP32-S3 WROOM
- DS18B20 contact temperature probe
- ambient temperature sensor context
- Bluetooth Low Energy

**iOS**

- SwiftUI
- Charts
- CoreBluetooth
- HealthKit / Apple Watch context
- local session persistence

**ML / AI**

- Python feature extraction
- transparent response classification
- OpenAI Responses API for plain-language coaching

## Current limitations

Krebb is an early prototype. The current system demonstrates signal collection, response classification, calibration workflow, and user explanation. It is not a medical device, does not diagnose disease, and does not yet estimate calories with validated accuracy.

The next step is collecting more labeled meals per person. With more data, Krebb can move from response classes toward personalized calorie-range estimation and meal-impact prediction.

## Vision

Food tracking should not require people to live like laboratory technicians. Krebb points toward a more convenient future: put on a small sensor, eat normally, and let your body help build the log.

Krebb makes nutrition tracking more automatic, more personal, and more realistic for everyday life.
