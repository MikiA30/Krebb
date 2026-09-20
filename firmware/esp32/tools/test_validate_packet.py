"""Tests for the Krebb One packet validator.

Run with:  pytest tools/ -q
"""

from __future__ import annotations

import pytest

from validate_packet import validate_packet

VALID = (
    b'{"timestampMs":1760000000000,"skinTemperatureC":33.42,'
    b'"ambientTemperatureC":24.0,"sensorQuality":0.91}'
)

UNAVAILABLE_SKIN = (
    b'{"timestampMs":1760000001000,"skinTemperatureC":null,'
    b'"ambientTemperatureC":24.9,"sensorQuality":0.0}'
)


def test_valid_packet():
    result = validate_packet(VALID)
    assert result.valid
    assert result.errors == []
    assert result.warnings == []
    assert result.packet["skinTemperatureC"] == pytest.approx(33.42)


def test_valid_packet_accepts_a_string_too():
    assert validate_packet(VALID.decode()).valid


def test_unavailable_skin_reading_is_valid():
    result = validate_packet(UNAVAILABLE_SKIN)
    assert result.valid, result.errors
    assert result.packet["skinTemperatureC"] is None
    assert result.packet["sensorQuality"] == 0.0


def test_both_temperatures_null_is_valid():
    raw = (
        b'{"timestampMs":1760000002000,"skinTemperatureC":null,'
        b'"ambientTemperatureC":null,"sensorQuality":0.0}'
    )
    assert validate_packet(raw).valid


def test_truncated_packet_is_reported_as_truncated():
    result = validate_packet(VALID[:40])
    assert not result.valid
    assert "truncated" in result.errors[0]
    assert "MTU" in result.errors[0]


def test_extra_field_is_a_warning_not_a_failure():
    raw = (
        b'{"timestampMs":1760000000000,"skinTemperatureC":33.42,'
        b'"ambientTemperatureC":24.0,"sensorQuality":0.91,"firmwareVersion":"0.1.0"}'
    )
    result = validate_packet(raw)
    assert result.valid
    assert len(result.warnings) == 1
    assert "firmwareVersion" in result.warnings[0]


def test_missing_required_key_fails():
    raw = b'{"timestampMs":1760000000000,"skinTemperatureC":33.42,"sensorQuality":0.91}'
    result = validate_packet(raw)
    assert not result.valid
    assert any("ambientTemperatureC" in e for e in result.errors)


@pytest.mark.parametrize(
    "raw",
    [
        b'{"timestampMs":"1760000000000","skinTemperatureC":33.42,"ambientTemperatureC":24.0,"sensorQuality":0.91}',
        b'{"timestampMs":1760000000000.5,"skinTemperatureC":33.42,"ambientTemperatureC":24.0,"sensorQuality":0.91}',
    ],
)
def test_non_integer_timestamp_fails(raw):
    result = validate_packet(raw)
    assert not result.valid
    assert any("timestampMs" in e for e in result.errors)


def test_wrong_temperature_type_fails():
    raw = (
        b'{"timestampMs":1760000000000,"skinTemperatureC":"33.42",'
        b'"ambientTemperatureC":24.0,"sensorQuality":0.91}'
    )
    result = validate_packet(raw)
    assert not result.valid
    assert any("skinTemperatureC" in e for e in result.errors)


def test_null_quality_fails():
    raw = (
        b'{"timestampMs":1760000000000,"skinTemperatureC":33.42,'
        b'"ambientTemperatureC":24.0,"sensorQuality":null}'
    )
    result = validate_packet(raw)
    assert not result.valid
    assert any("sensorQuality" in e for e in result.errors)


@pytest.mark.parametrize("quality", [1.5, -0.1, 2])
def test_out_of_range_quality_fails(quality):
    raw = (
        '{"timestampMs":1760000000000,"skinTemperatureC":33.42,'
        f'"ambientTemperatureC":24.0,"sensorQuality":{quality}}}'
    )
    result = validate_packet(raw)
    assert not result.valid
    assert any("sensorQuality" in e for e in result.errors)


def test_sensor_sentinel_temperature_fails():
    raw = (
        b'{"timestampMs":1760000000000,"skinTemperatureC":-127.0,'
        b'"ambientTemperatureC":24.0,"sensorQuality":0.5}'
    )
    result = validate_packet(raw)
    assert not result.valid
    assert any("-127" in e for e in result.errors)


def test_non_utf8_payload_fails():
    result = validate_packet(b"\xff\xfe\x00 not utf-8")
    assert not result.valid
    assert "UTF-8" in result.errors[0]


def test_empty_payload_fails():
    assert not validate_packet(b"").valid


def test_non_object_json_fails():
    result = validate_packet(b"[1,2,3]")
    assert not result.valid
    assert "expected a JSON object" in result.errors[0]


def test_timestamp_must_increase_across_a_stream():
    first = validate_packet(VALID)
    assert first.valid
    previous = first.packet["timestampMs"]

    # Same timestamp again: a repeat, which would mean a stale packet.
    repeat = validate_packet(VALID, previous_timestamp_ms=previous)
    assert not repeat.valid
    assert any("did not increase" in e for e in repeat.errors)

    # A later one is fine.
    assert validate_packet(UNAVAILABLE_SKIN, previous_timestamp_ms=previous).valid


def test_boolean_is_not_accepted_as_a_number():
    raw = (
        b'{"timestampMs":1760000000000,"skinTemperatureC":true,'
        b'"ambientTemperatureC":24.0,"sensorQuality":0.91}'
    )
    assert not validate_packet(raw).valid
