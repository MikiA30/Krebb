"""Validation for the Krebb One BLE measurement packet.

Pure functions, no BLE imports, so they can be unit-tested without hardware.

The contract (docs/sensor-protocol.md, docs/BLE_PROTOCOL.md):

    {"timestampMs":1760000000000,"skinTemperatureC":33.42,
     "ambientTemperatureC":24.0,"sensorQuality":0.91}

- exactly these four keys are required
- timestampMs is an integer (Unix milliseconds), and must increase
- skinTemperatureC and ambientTemperatureC are a number or null (Celsius)
- sensorQuality is a number in [0.0, 1.0]
- unknown extra keys are a WARNING, not a failure: the contract explicitly
  allows future fields, and the iOS app must tolerate them

Krebb One is a research prototype, not a medical device. These values are
sensor readings, not clinical measurements.
"""

from __future__ import annotations

import json
from dataclasses import dataclass, field

REQUIRED_KEYS = ("timestampMs", "skinTemperatureC", "ambientTemperatureC", "sensorQuality")
NULLABLE_TEMPERATURE_KEYS = ("skinTemperatureC", "ambientTemperatureC")

# Sensor sentinels that must never appear on the wire as a reading.
FORBIDDEN_TEMPERATURES = (-127.0, 85.0)


@dataclass
class ValidationResult:
    valid: bool
    errors: list[str] = field(default_factory=list)
    warnings: list[str] = field(default_factory=list)
    packet: dict | None = None

    @property
    def summary(self) -> str:
        if self.valid and not self.warnings:
            return "ok"
        parts = [f"ERROR: {e}" for e in self.errors]
        parts += [f"warning: {w}" for w in self.warnings]
        return "; ".join(parts)


def validate_packet(raw: bytes | bytearray | str, previous_timestamp_ms: int | None = None) -> ValidationResult:
    """Validates one notification payload.

    `previous_timestamp_ms` enables the monotonic-timestamp check across a
    stream; pass None for the first packet or for a standalone check.
    """
    errors: list[str] = []
    warnings: list[str] = []

    if isinstance(raw, (bytes, bytearray)):
        try:
            text = bytes(raw).decode("utf-8")
        except UnicodeDecodeError as exc:
            return ValidationResult(False, [f"not valid UTF-8: {exc}"])
    else:
        text = raw

    stripped = text.strip()
    if not stripped:
        return ValidationResult(False, ["empty payload"])

    # A truncated notification is the classic symptom of an MTU that is too
    # small, so it is worth naming explicitly rather than just "bad JSON".
    if stripped.startswith("{") and not stripped.endswith("}"):
        return ValidationResult(
            False,
            [f"packet looks truncated ({len(text)} bytes, no closing brace) - check the negotiated MTU"],
        )

    try:
        packet = json.loads(stripped)
    except json.JSONDecodeError as exc:
        return ValidationResult(False, [f"invalid JSON: {exc}"])

    if not isinstance(packet, dict):
        return ValidationResult(False, [f"top level is {type(packet).__name__}, expected a JSON object"])

    for key in REQUIRED_KEYS:
        if key not in packet:
            errors.append(f"missing required key '{key}'")

    for key in sorted(set(packet) - set(REQUIRED_KEYS)):
        warnings.append(f"unknown extra key '{key}' (allowed by the contract, but unexpected)")

    # --- timestampMs ---------------------------------------------------------
    timestamp = packet.get("timestampMs")
    if "timestampMs" in packet:
        # bool is a subclass of int; a JSON true here would be a type error.
        if isinstance(timestamp, bool) or not isinstance(timestamp, int):
            errors.append(f"timestampMs must be an integer, got {type(timestamp).__name__}")
        elif timestamp <= 0:
            errors.append(f"timestampMs must be positive, got {timestamp}")
        elif previous_timestamp_ms is not None and timestamp <= previous_timestamp_ms:
            errors.append(
                f"timestampMs did not increase: {timestamp} after {previous_timestamp_ms}"
            )

    # --- temperatures --------------------------------------------------------
    for key in NULLABLE_TEMPERATURE_KEYS:
        if key not in packet:
            continue
        value = packet[key]
        if value is None:
            continue  # null is the contract's "unavailable"
        if isinstance(value, bool) or not isinstance(value, (int, float)):
            errors.append(f"{key} must be a number or null, got {type(value).__name__}")
            continue
        if any(abs(value - sentinel) < 1e-9 for sentinel in FORBIDDEN_TEMPERATURES):
            errors.append(f"{key} is the sensor sentinel {value}, which must never be sent")

    # --- sensorQuality -------------------------------------------------------
    if "sensorQuality" in packet:
        quality = packet["sensorQuality"]
        if quality is None:
            errors.append("sensorQuality must always be a number, got null")
        elif isinstance(quality, bool) or not isinstance(quality, (int, float)):
            errors.append(f"sensorQuality must be a number, got {type(quality).__name__}")
        elif not (0.0 <= quality <= 1.0):
            errors.append(f"sensorQuality must be within [0.0, 1.0], got {quality}")

    return ValidationResult(not errors, errors, warnings, packet if not errors else None)
