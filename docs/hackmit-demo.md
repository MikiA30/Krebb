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
5. Open the saved session detail and show Krebb Coach explaining the response in consumer language.
6. Run the exported session through the transparent response classifier or OpenAI coach script if judges ask for the pipeline.
7. Load a saved longer session if needed to explain the 60-90 minute thermogenic-response story.
8. Explain the future vision: personal calibration can refine meal tracking without pretending to provide exact caloric truth today.

Keep a pre-recorded complete session available for the demo. It is a reliability fallback, not a substitute for labeling the result accurately.

## Claim boundary

Krebb can show early response classes and calibration points during the hackathon. It should not claim validated calorie estimation from two or three sessions.


## Sponsor-track framing

- Espressif: Krebb uses ESP32-S3 hardware as the bridge between the physical body signal and the iPhone experience.
- OpenAI: Krebb Coach can use the OpenAI Responses API to turn extracted sensor features into a concise explanation for the user while preserving the limitation that the model is not a validated calorie estimator.
- Long Lake: the user can eat something, wait 15 minutes, and see an AI-assisted explanation of their own body signal instead of a generic nutrition lecture.

For the OpenAI demo path, keep the API key outside the app and run `ml/scripts/openai_coach.py` from the terminal against an exported JSON session. The iOS app keeps a local deterministic coach card so the demo does not fail if Wi-Fi or API setup is unavailable.
