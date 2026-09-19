# HackMIT experiment plan

## Research question

Can synchronized, within-person changes in heart rate, contact temperature, ambient conditions, and activity distinguish preliminary post-meal response classes?

## Scope

- This is an exploratory prototype, not a calorimeter, medical device, or validated calorie estimator.
- Use contact temperature from TMP117 and ambient temperature/humidity from BME280.
- Use MAX30102 or permitted Apple Watch / HealthKit data for heart rate and activity.
- A thermal camera is not required.

## Session procedure

1. Record a 10-15 minute quiet baseline.
2. Record the meal label for the controlled experiment: water/no meal, small, moderate, or large.
3. Collect a post-meal window while noting activity, caffeine, and interruptions.
4. Store the raw session locally and pseudonymize it before sharing with the team.
5. Extract features only after checking timestamp alignment and signal quality.

## Initial features

- Baseline heart rate and peak delta heart rate
- Heart-rate area under the curve
- Baseline contact temperature and peak delta temperature
- Temperature area under the curve and time to peak
- Ambient temperature and humidity
- Activity and step count
- Measurement quality and missing-data flags

## Evaluation

Use leave-one-session-out evaluation only when enough sessions exist. With a very small dataset, present the model as a demonstration of the pipeline and show the raw traces alongside the prediction.
