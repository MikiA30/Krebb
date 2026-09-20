# HackMIT demo

## Current proof

- Krebb One streams DS18B20/ambient packets over BLE to the iPhone app.
- The iPhone app can record live baseline changes and save them locally.
- Apple Health context is optional and stays separate from ESP32 packets.

## Demo flow

1. Open Krebb and connect Krebb One.
2. Show a 1-3 minute live baseline with contact temperature and ambient context.
3. Begin an observation named after the food/event.
4. Show the saved session trend and extracted features.
5. Run the exported session through the transparent response classifier.
6. Load a saved longer session if needed to explain the 60-90 minute thermogenic-response story.
7. Explain the future vision: personal calibration can refine meal tracking without pretending to provide exact caloric truth today.

Keep a pre-recorded complete session available for the demo. It is a reliability fallback, not a substitute for labeling the result accurately.

## Claim boundary

Krebb can show early response classes and calibration points during the hackathon. It should not claim validated calorie estimation from two or three sessions.
