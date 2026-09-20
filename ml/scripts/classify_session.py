#!/usr/bin/env python3
"""Classify a Krebb exported session with transparent hackathon heuristics."""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from statistics import mean


def load_session(path: Path) -> dict:
    with path.open("r", encoding="utf-8") as handle:
        return json.load(handle)


def feature_value(session: dict, key: str) -> float | None:
    value = (session.get("features") or {}).get(key)
    return value if isinstance(value, int | float) else None


def fallback_features(session: dict) -> dict:
    baseline = session.get("baselineSkinTemperatureC")
    samples = session.get("deviceSamples") or []
    observation = [
        sample for sample in samples
        if sample.get("phase") == "observation" and sample.get("skinTemperatureC") is not None
    ]
    deltas = []
    if isinstance(baseline, int | float):
        deltas = [sample["skinTemperatureC"] - baseline for sample in observation]
    qualities = [
        sample.get("sensorQuality") for sample in samples
        if isinstance(sample.get("sensorQuality"), int | float)
    ]
    return {
        "peakDeltaSkinTemperatureC": max(deltas) if deltas else None,
        "latestDeltaSkinTemperatureC": deltas[-1] if deltas else None,
        "averageSensorQuality": mean(qualities) if qualities else None,
        "observationSampleCount": len(observation),
    }


def classify(peak_delta: float | None, auc: float | None, quality: float | None) -> tuple[str, str]:
    if peak_delta is None:
        return "insufficient_data", "No usable observation skin-temperature samples were found."
    if quality is not None and quality < 0.45:
        return "low_quality", "Sensor quality was too low for a confident response class."
    if peak_delta < 0.35 and (auc is None or auc < 200):
        return "control_like", "The trace stayed close to baseline."
    if peak_delta < 0.8:
        return "small_early_response", "The trace moved above baseline, but only modestly."
    if peak_delta < 1.5:
        return "moderate_early_response", "The trace showed a clear early rise from baseline."
    return "high_early_response", "The trace showed a strong early rise from baseline."


def summarize(session: dict, known_calories: int | None) -> dict:
    fallbacks = fallback_features(session)
    peak_delta = feature_value(session, "peakDeltaSkinTemperatureC")
    latest_delta = feature_value(session, "latestDeltaSkinTemperatureC")
    auc = feature_value(session, "temperatureAreaCelsiusSeconds")
    quality = feature_value(session, "averageSensorQuality")
    observation_samples = feature_value(session, "observationSampleCount")
    duration = feature_value(session, "durationSeconds")

    peak_delta = peak_delta if peak_delta is not None else fallbacks["peakDeltaSkinTemperatureC"]
    latest_delta = latest_delta if latest_delta is not None else fallbacks["latestDeltaSkinTemperatureC"]
    quality = quality if quality is not None else fallbacks["averageSensorQuality"]
    observation_samples = observation_samples if observation_samples is not None else fallbacks["observationSampleCount"]
    response_class, explanation = classify(peak_delta, auc, quality)

    output = {
        "note": (session.get("note") or "").strip(),
        "source": session.get("source"),
        "knownCalories": known_calories,
        "responseClass": response_class,
        "explanation": explanation,
        "features": {
            "baselineSkinTemperatureC": session.get("baselineSkinTemperatureC"),
            "peakDeltaSkinTemperatureC": peak_delta,
            "latestDeltaSkinTemperatureC": latest_delta,
            "temperatureAreaCelsiusSeconds": auc,
            "averageSensorQuality": quality,
            "observationSampleCount": observation_samples,
            "durationSeconds": duration,
        },
        "claimBoundary": "This is an experimental within-person response class, not a validated calorie estimate.",
    }
    if known_calories is not None:
        output["calibrationUse"] = (
            f"Use this as one labeled calibration point at {known_calories} calories; "
            "more labeled sessions are required before calorie-range estimation."
        )
    return output


def main() -> None:
    parser = argparse.ArgumentParser(description="Classify one Krebb exported session.")
    parser.add_argument("session_json", type=Path)
    parser.add_argument("--known-calories", type=int)
    args = parser.parse_args()

    print(json.dumps(summarize(load_session(args.session_json), args.known_calories), indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
