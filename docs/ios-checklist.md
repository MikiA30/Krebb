# iOS team checklist

Status date: 2026-09-20

## Done

- Native SwiftUI shell with Today, Check, Journal, and Sensors.
- Physical iPhone build runs.
- Apple Health permission and latest Watch-synced context can be attached to sessions.
- Simulation flow records, resumes, saves, and appears in Journal.
- Krebb One BLE scan, explicit device connection, notification subscription, and live packet recording work on iPhone.
- Live baseline changes from the DS18B20/ambient ESP32 stream have been observed on iPhone.
- Completed sessions show local response features and debug JSON export for the future pipeline.
- Saved drafts survive relaunch; unfinished drafts can be discarded.
- Live BLE is the primary path once packets are available; simulation becomes visually secondary.
- Active checks show a clear live/simulation badge.
- Baseline now guides the user to hold still for 3 readings before observation.
- The food/event label step is explicit before observation starts.
- Real-session charts show a cleaner trend with only the latest sample highlighted.

## Next before ML

- Save one clean live BLE session named `Hardware baseline test` as demo proof.
- Confirm reconnect behavior after app backgrounding, ESP32 power-cycle, and `Stop sensor`.
- Decide the hackathon demo script: live 1-3 minute baseline plus saved longer session for post-meal story.

## Later

- Add a customer-facing result screen that hides raw feature names.
- Add export/import tooling for longer labeled sessions.
- Add pipeline output display once the non-ML feature export has enough real data.
- Consider a backend only if remote inference or protected API calls become necessary.
