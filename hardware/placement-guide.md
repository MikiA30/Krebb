# Placement and electrical-safety guide

Krebb One is a research prototype. It is **not a medical device**, and it does not measure calories or clinical body temperature. The DS18B20 gives a skin-contact temperature **trend**; the useful signal is the change from your own baseline within one session.

## 1. Pick one skin location and keep it

- Choose **one** location and use it for every baseline and post-meal measurement. The collarbone / upper-chest area is a candidate. Choose the spot where your enclosure and wiring give **safe, stable contact**.
- Write the location down once (for example "left upper chest, 3 cm below the collarbone") and reuse the same wording every session. Moving the sensor between sessions changes the reading more than the meal does.

## 2. Attaching the DS18B20 for consistent contact

- Use a **sealed waterproof probe**, or a bare TO-92 sensor that is **fully insulated** (heat-shrunk over the body and every lead).
- **No exposed lead or solder joint against skin.** Cover every conductor.
- Hold the sensor against skin with **medical-grade tape** or a **snug band**. Aim for the same light, steady pressure each time. Too loose loses contact; too tight changes local blood flow and the reading.
- Add **strain relief**: tape the cable to the skin or clothing a few centimeters behind the sensor so a tug on the wire does not pull the sensor off or load the joint.
- Keep the breadboard, ESP32 board, and any bare wiring **off the body**. Only the insulated sensor and its cable touch the person.
- If you are not sure the probe's stainless tube is isolated from its wires, run the continuity test in [wiring/breadboard-build.md](wiring/breadboard-build.md) first.
- Attachment materials (tape, band, heat-shrink) are not in the BOM; use what you already have.

## 3. Baseline: stay still

- Stay **still for the whole 10 to 15 minute baseline**. Sit or recline in the same posture each session.
- The reading takes a while to settle after placement; a stabilizing period is expected, and the app's `stabilizing` quality state reflects it.
- Avoid handling the sensor or shifting the tape during the baseline and the observation window. Note any interruption (see section 6).

## 4. The DHT11 stays in open room air

- Keep the DHT11 **in open room air, away from your body** and **away from the ESP32's own heat**. Do not tuck it under clothing, against the enclosure wall, or next to the ESP32 module or the USB port.
- Do not point it at a vent, window, or direct sunlight.
- Its job is to notice **room changes**: an HVAC cycle, a door or window opening, the device being handled, contact loss, or the sensor being exposed to different air. It is coarse (about 1 °C steps, about ±2 °C accuracy) and is context only, not a measurement to correct against. See [bom/bom.md](bom/bom.md).

## 5. Electrical safety for anything that touches skin

- The circuit runs at **3.3 V at low current**, so the risk is low, but treat it carefully anyway.
- **Insulate every exposed conductor** near skin. Use a sealed probe or a fully insulated sensor.
- Power the board **only from a laptop USB port or a battery power bank**. **Never** use a non-isolated mains supply.
- Prefer a **battery power bank** for skin sessions if your laptop is charging from mains. Check that the bank stays on with this small load (**UNVERIFIED (check on the bench)**).
- Do the multimeter checks in [wiring/breadboard-build.md](wiring/breadboard-build.md) and confirm the DS18B20 pinout **before** the first power-on. A reversed DS18B20 can destroy itself and heat up quickly.
- **Stop immediately** (unplug USB, remove the sensor) if there is **any skin irritation, redness, or discomfort, or any unexpected warmth** from the sensor, cable, or board.
- Keep the electronics dry, and keep the breadboard and ESP32 board off the body entirely.

## 6. What to note at each session (no personal data)

Record these in the session notes. **Do not** record or commit names, dates of birth, photos, account IDs, health records, or anything that identifies a person. Use an anonymous session label.

| Field | Example |
| --- | --- |
| Session label (anonymous) | S07 |
| Skin location (same wording every time) | left upper chest |
| Time since eating | 3 h fasted / 45 min after meal |
| Meal label (per the experiment plan) | water / no meal, small, moderate, large |
| Room conditions | approx. room temperature, HVAC on/off, window or door open, fan running |
| Posture | seated, back supported |
| Interruptions | stood up at minute 9, sensor tape re-pressed at minute 12 |
| Caffeine or exercise beforehand | none / coffee 1 h before |
| Hardware notes | probe vs bare sensor, tape vs band, power source (laptop / power bank) |

Raw sessions stay local and are pseudonymized before sharing, per [../docs/experiment-plan.md](../docs/experiment-plan.md). Nothing personally identifying goes into Git.
