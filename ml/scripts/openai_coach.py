#!/usr/bin/env python3
"""Generate a short Krebb Coach explanation from an exported session.

Set OPENAI_API_KEY to call the OpenAI Responses API. Use --dry-run to print the
prompt without making a network request.
"""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
import urllib.request
from pathlib import Path


def classify_locally(session_json: Path, known_calories: int | None) -> dict:
    command = [sys.executable, str(Path(__file__).with_name("classify_session.py")), str(session_json)]
    if known_calories is not None:
        command.extend(["--known-calories", str(known_calories)])
    result = subprocess.run(command, check=True, capture_output=True, text=True)
    return json.loads(result.stdout)


def build_prompt(summary: dict) -> str:
    features = summary["features"]
    return f"""You are Krebb Coach inside an iPhone health prototype.
Write for a normal consumer, not an engineer.

Output rules:
- Plain text only. No Markdown, no asterisks, no bold, no headings with colons, no bullet symbols, and no hyphen bullets.
- Write exactly 3 short numbered lines starting with "1)", "2)", and "3)".
- Each line should be one sentence under 22 words.
- Be specific enough to be useful, but avoid dumping raw metrics.
- Mention the meal name naturally.
- If known calories are present, mention that this is saved as a labeled example.
- Do not give medical advice.
- Do not claim calorie estimation.

Meaning to convey:
1) What changed after the meal.
2) What the user should do next to make Krebb smarter.
3) The limitation in friendly language.

Session:
label: {summary.get('note') or 'unlabeled'}
known calories: {summary.get('knownCalories')}
response class: {summary.get('responseClass')}
explanation: {summary.get('explanation')}
baseline skin C: {features.get('baselineSkinTemperatureC')}
peak delta C: {features.get('peakDeltaSkinTemperatureC')}
latest delta C: {features.get('latestDeltaSkinTemperatureC')}
temperature AUC C*s: {features.get('temperatureAreaCelsiusSeconds')}
average sensor quality: {features.get('averageSensorQuality')}
observation samples: {features.get('observationSampleCount')}
"""


def call_openai(prompt: str, model: str) -> str:
    api_key = os.environ.get("OPENAI_API_KEY")
    if not api_key:
        raise SystemExit("OPENAI_API_KEY is not set. Re-run with --dry-run or export an API key.")
    body = json.dumps({"model": model, "store": False, "input": prompt}).encode("utf-8")
    request = urllib.request.Request(
        "https://api.openai.com/v1/responses",
        data=body,
        headers={
            "Authorization": f"Bearer {api_key}",
            "Content-Type": "application/json",
        },
        method="POST",
    )
    with urllib.request.urlopen(request, timeout=30) as response:
        payload = json.loads(response.read().decode("utf-8"))
    if payload.get("output_text"):
        return payload["output_text"]
    chunks: list[str] = []
    for item in payload.get("output", []):
        for content in item.get("content", []):
            if content.get("type") == "output_text" and content.get("text"):
                chunks.append(content["text"])
    return "\n".join(chunks).strip() or json.dumps(payload, indent=2)


def main() -> None:
    parser = argparse.ArgumentParser(description="Create a Krebb Coach explanation with optional OpenAI API output.")
    parser.add_argument("session_json", type=Path)
    parser.add_argument("--known-calories", type=int)
    parser.add_argument("--model", default=os.environ.get("OPENAI_MODEL", "gpt-5.6-luna"))
    parser.add_argument("--dry-run", action="store_true")
    args = parser.parse_args()

    summary = classify_locally(args.session_json, args.known_calories)
    prompt = build_prompt(summary)
    if args.dry_run:
        print(prompt)
        return
    print(call_openai(prompt, args.model))


if __name__ == "__main__":
    main()
