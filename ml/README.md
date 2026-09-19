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
