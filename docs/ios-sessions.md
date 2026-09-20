# iOS session recording

The app records foreground sessions through baseline, observation, and completion. It supports simulation and the Krebb One BLE stream. It is not a physiological experiment.

## Try it

1. Open Check and scan for Krebb One, or start a simulation.
2. Wait for at least three generated samples, optionally add a note, then begin observation.
3. Allow at least one observation sample, then finish and save.
4. Open Journal and select the simulation. Relaunch the app and verify it remains available.
5. Relaunch during a check to resume the autosaved draft. Recording gaps are not filled with invented samples.

Today displays the active recording or latest saved session. Before any recording exists, Today retains its labeled example curve. Session provenance is persisted in each record and remains visible in Journal.

## Storage and recovery

`SessionStore` writes a version-1 Codable JSON archive at `Application Support/KrebbSessions/sessions.json` inside the iOS app container. It contains the current draft and completed sessions. Writes are atomic and use complete file protection; the directory is excluded from device cloud backup. No cloud upload or export is implemented.

Every accepted reading and state transition is saved. Failed writes leave the previous in-memory and on-disk state intact and show an error. An unreadable or unsupported archive is preserved and writes are blocked instead of replacing the archive with an empty one. Notes are also saved while editing.

The simulator produces one temperature sample per foreground second. The baseline is the arithmetic mean of pre-observation nonmissing skin readings and becomes fixed at the observation boundary. Three samples are a simulation-only convenience, not the real experiment's baseline criterion. Charts use actual timestamps and split lines over missing skin data or gaps longer than three seconds.

BLE packets are decoded from the Krebb One measurement characteristic and saved only while a sensor session is active. The app charts phone arrival time and retains the ESP32 `timestampMs` on each reading because the current firmware clock may be unsynced.

## Health context

HealthKit access remains explicit in Sensors. Already-loaded optional Health snapshots are attached at simulation start/finish, or via the Attach button. Identical consecutive snapshots are deduplicated. The heart-rate measurement timestamp and source are retained; snapshot capture time is separate. These readings can predate the session and are context, not a synchronized live heart-rate stream. The step value is an individual HealthKit sample, not today's total.

## Next integration gate

Validate BLE against the physical board, then tune baseline duration and quality gates from real skin-contact data. Live sessions still need cancellation/interruption handling. Export should explicitly map the local archive to the shared ML schema rather than assuming the two formats already match.

Tests cover draft recovery, completion/reload, fixed baseline, Health timestamp retention and deduplication, missing/invalid values, corrupt archives, write failures, and the UI save/relaunch flow.
