#!/usr/bin/env python3
"""Verify include/pins.h against the pin contract in hardware/wiring/pin-map.md.

The pin map is the hardware source of truth. If the firmware and the wiring
document ever disagree about which GPIO a sensor is on, the bench session is
wasted chasing a phantom sensor fault, so this fails loudly instead.

Usage:
    python3 scripts/check_pins.py [--quiet]

Exit status 0 when they agree, 1 when they do not (or a file cannot be read).
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

FIRMWARE_DIR = Path(__file__).resolve().parent.parent
REPO_ROOT = FIRMWARE_DIR.parent.parent

PINS_HEADER = FIRMWARE_DIR / "include" / "pins.h"
PIN_MAP_DOC = REPO_ROOT / "hardware" / "wiring" / "pin-map.md"

# The pin names that must exist and match in both places.
REQUIRED_PINS = ("PIN_SKIN_ONEWIRE", "PIN_AMBIENT_DHT11")

# Matches the pin-map.md contract lines, e.g.
#   PIN_SKIN_ONEWIRE   = 4     # GPIO4, DS18B20 DQ, ...
DOC_PIN_RE = re.compile(r"^\s*(PIN_[A-Z0-9_]+)\s*=\s*(\d+)\s*(?:#.*)?$", re.MULTILINE)

# Matches the C++ declaration, e.g.
#   constexpr int PIN_SKIN_ONEWIRE = 4;
HEADER_PIN_RE = re.compile(
    r"^\s*constexpr\s+int\s+(PIN_[A-Z0-9_]+)\s*=\s*(\d+)\s*;", re.MULTILINE
)


def parse(path: Path, pattern: re.Pattern[str], label: str) -> dict[str, int]:
    try:
        text = path.read_text(encoding="utf-8")
    except OSError as exc:
        print(f"ERROR: cannot read {label} ({path}): {exc}", file=sys.stderr)
        raise SystemExit(1) from exc

    found = {name: int(value) for name, value in pattern.findall(text)}
    if not found:
        print(f"ERROR: no pin definitions found in {label} ({path})", file=sys.stderr)
        raise SystemExit(1)
    return found


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--quiet", action="store_true", help="only print on failure")
    args = parser.parse_args()

    doc_pins = parse(PIN_MAP_DOC, DOC_PIN_RE, "pin map")
    header_pins = parse(PINS_HEADER, HEADER_PIN_RE, "pins.h")

    problems: list[str] = []

    for name in REQUIRED_PINS:
        if name not in doc_pins:
            problems.append(f"{name} is missing from the Firmware pin contract in {PIN_MAP_DOC.name}")
        if name not in header_pins:
            problems.append(f"{name} is missing from {PINS_HEADER.name}")

    for name in sorted(set(doc_pins) | set(header_pins)):
        in_doc = doc_pins.get(name)
        in_header = header_pins.get(name)
        if in_doc is None:
            problems.append(f"{name}=GPIO{in_header} is in pins.h but not in the pin map")
        elif in_header is None:
            problems.append(f"{name}=GPIO{in_doc} is in the pin map but not in pins.h")
        elif in_doc != in_header:
            problems.append(
                f"{name} disagrees: pin map says GPIO{in_doc}, pins.h says GPIO{in_header}"
            )

    if problems:
        print("PIN CONTRACT MISMATCH", file=sys.stderr)
        for problem in problems:
            print(f"  - {problem}", file=sys.stderr)
        print(f"\n  pin map: {PIN_MAP_DOC}", file=sys.stderr)
        print(f"  header : {PINS_HEADER}", file=sys.stderr)
        return 1

    if not args.quiet:
        print("pin contract OK:")
        for name in REQUIRED_PINS:
            print(f"  {name} = GPIO{header_pins[name]}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
