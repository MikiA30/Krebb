# Submission game plan

## While a live food trial records

- Do not background or rebuild over the phone running the active session.
- Prepare the story around the completed control and donut sessions.
- Keep one person watching the phone until the trial reaches 15 minutes.
- After saving, export the JSON and run the session through the local classifier script.

## Demo story

1. Krebb One streams contact temperature and ambient context over BLE.
2. The iPhone records a personal baseline, then marks the first bite as observation start.
3. A no-meal control stayed comparatively flat.
4. A 240-calorie donut created a strong early response over 15 minutes.
5. The current model classifies response strength; calorie estimation requires more labeled personal sessions.

## Response classifier

The hackathon classifier is intentionally transparent:

- `control_like`: trace stays close to baseline.
- `small_early_response`: modest rise.
- `moderate_early_response`: clear rise.
- `high_early_response`: strong early rise.
- `low_quality`: sensor quality too low.

Run it against an exported session:

```bash
python3 ml/scripts/classify_session.py path/to/session.json --known-calories 240
```

Do not call the output a validated calorie estimate. Call it a personalized calibration point.
