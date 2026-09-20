#pragma once

#include <stdint.h>

// DHT11 sample validation for Krebb One.
//
// The DHT11 protocol carries a checksum, but the checksum only proves the
// five bytes arrived intact - it says nothing about whether the sensor was in
// a fit state to measure. An all-zero frame (0.0 degC, 0 %RH) has a valid
// checksum of 0 and is exactly what the part emits while it is mis-wired,
// half-powered or still recovering, so it reaches the firmware dressed up as
// a plausible winter room. Sent as ambientTemperatureC it would be a
// fabricated measurement, which is worse than the null the contract allows.
//
// So a sample is cross-checked against the part's own datasheet envelope
// before it is believed. Humidity is never transmitted, but it is the only
// second opinion the sensor offers: a frame claiming humidity outside the
// 20-90 %RH the part is specified for did not come from a healthy sensor, and
// its temperature is discarded with it.
//
// A rejected sample counts as a failed read, identical to a timeout: it does
// not update the last valid reading or its timestamp, so the existing
// freshness rule expires the previous value on schedule and the packet then
// carries null.
//
// Pure C++: no Arduino headers, so it is unit-tested on the host.

namespace krebb {

// Why a sample was rejected. Serial log only; never transmitted.
enum class AmbientReject : uint8_t {
  NONE,              // the sample is usable
  NOT_FINITE,        // NaN/inf temperature or humidity
  ALL_ZERO_FRAME,    // 0.0 degC and 0 %RH: a checksum-valid non-measurement
  HUMIDITY_RANGE,    // outside the datasheet 20-90 %RH
  TEMPERATURE_RANGE, // outside the datasheet 0-50 degC
};

struct AmbientValidation {
  bool valid = false;
  AmbientReject reject = AmbientReject::NOT_FINITE;
};

// Decides whether one DHT11 frame may become the new last-valid reading.
AmbientValidation validateAmbientSample(float temperatureC, float humidityPercent);

// Human-readable explanation for the serial log.
const char* ambientRejectMessage(AmbientReject reject);

}  // namespace krebb
