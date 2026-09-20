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

## OpenAI coach script

`scripts/openai_coach.py` wraps the same classifier output in a short consumer explanation using the OpenAI Responses API when `OPENAI_API_KEY` is available. It does not store an API key in the repo or iOS app.

Dry-run the prompt without a network call:

```bash
python3 ml/scripts/openai_coach.py data/raw/sandwich.json --known-calories 450 --dry-run
```

Run the API-backed version:

```bash
export OPENAI_API_KEY="..."
python3 ml/scripts/openai_coach.py data/raw/sandwich.json --known-calories 450
```

The prompt asks the model to explain the signal, calibration value, and limitation without medical advice or validated calorie claims.
