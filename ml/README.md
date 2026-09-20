# Krebb ML pipeline

The ML pipeline turns synchronized session data into interpretable features and a preliminary within-person meal-response model.

```text
data/raw/        Private, unprocessed experiment recordings; ignored by Git
data/processed/  Reproducible derived datasets when safe to share
notebooks/       Exploratory analysis only
src/             Reusable feature, model, and utility code
scripts/         Repeatable command-line entry points
```

Start with response classification. Do not present a tiny hackathon dataset as a validated calorie regression model.

## Hackathon classifier

`scripts/classify_session.py` reads one exported iOS session JSON and emits a transparent response class plus the feature values used. It supports an optional known-calorie label so sessions like `Donut: 240 cal` can become calibration points later.

Example:

```bash
python3 ml/scripts/classify_session.py data/raw/donut.json --known-calories 240
```

The output is an experimental response class, not a validated calorie estimate.
