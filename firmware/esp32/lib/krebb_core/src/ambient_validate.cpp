#include "ambient_validate.h"

#include <math.h>

#include "krebb_params.h"

namespace krebb {
namespace {

using namespace krebb::params;

AmbientValidation rejected(AmbientReject reason) {
  AmbientValidation out;
  out.valid = false;
  out.reject = reason;
  return out;
}

}  // namespace

AmbientValidation validateAmbientSample(float temperatureC, float humidityPercent) {
  // NaN first: every comparison below is false for NaN, so an unchecked NaN
  // would slip through every range test.
  if (!isfinite(temperatureC) || !isfinite(humidityPercent)) {
    return rejected(AmbientReject::NOT_FINITE);
  }

  // The all-zero frame. Checked before the range tests purely so the log names
  // the real fault - 0 %RH would otherwise be reported as a humidity range
  // failure, which hides the wiring problem behind a sensor-spec complaint.
  if (temperatureC == 0.0f && humidityPercent == 0.0f) {
    return rejected(AmbientReject::ALL_ZERO_FRAME);
  }

  if (humidityPercent < AMBIENT_HUMIDITY_MIN_PCT ||
      humidityPercent > AMBIENT_HUMIDITY_MAX_PCT) {
    return rejected(AmbientReject::HUMIDITY_RANGE);
  }

  if (temperatureC < AMBIENT_SANITY_MIN_C || temperatureC > AMBIENT_SANITY_MAX_C) {
    return rejected(AmbientReject::TEMPERATURE_RANGE);
  }

  AmbientValidation out;
  out.valid = true;
  out.reject = AmbientReject::NONE;
  return out;
}

const char* ambientRejectMessage(AmbientReject reject) {
  switch (reject) {
    case AmbientReject::NONE:
      return "";
    case AmbientReject::NOT_FINITE:
      return "DHT11 returned NaN";
    case AmbientReject::ALL_ZERO_FRAME:
      return "DHT11 all-zero frame (0.0C / 0%RH) - checksum-valid but not a measurement";
    case AmbientReject::HUMIDITY_RANGE:
      return "DHT11 humidity outside the 20-90 %RH operating range - sample discarded";
    case AmbientReject::TEMPERATURE_RANGE:
      return "DHT11 reading outside the 0-50 degC operating range";
  }
  return "DHT11 sample rejected";
}

}  // namespace krebb
