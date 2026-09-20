#include "packet.h"

#include <math.h>
#include <stdio.h>

#include "krebb_params.h"

namespace krebb {
namespace {

using namespace krebb::params;

// Longest field rendering we ever produce ("-55.00" plus slack).
constexpr size_t FIELD_BUFFER_SIZE = 16;

constexpr const char* JSON_NULL = "null";

// Formats `value` with a fixed number of decimals WITHOUT touching the C
// locale.
//
// WHY not snprintf("%.2f"): printf's %f honours the current locale's decimal
// separator. Under a comma locale it would emit {"skinTemperatureC":33,42},
// which is not merely ugly - it is invalid JSON and would silently break the
// iOS parser. Rendering from a scaled integer cannot do that, and it also
// makes the output byte-stable for the exact-string unit tests.
//
// Returns false if the value is not finite or does not fit.
bool writeFixed(char* out, size_t outSize, double value, int decimals) {
  if (!isfinite(value)) {
    return false;
  }

  double scale = 1.0;
  for (int i = 0; i < decimals; ++i) {
    scale *= 10.0;
  }

  const double scaled = value * scale;
  // Keep well inside the range where llround is exact and meaningful.
  if (scaled > 9.0e15 || scaled < -9.0e15) {
    return false;
  }

  const long long rounded = llround(scaled);
  const bool negative = rounded < 0;
  // Negate in unsigned space so LLONG_MIN cannot trap.
  const unsigned long long magnitude =
      negative ? (0ULL - static_cast<unsigned long long>(rounded))
               : static_cast<unsigned long long>(rounded);

  unsigned long long divisor = 1;
  for (int i = 0; i < decimals; ++i) {
    divisor *= 10ULL;
  }

  const unsigned long long whole = magnitude / divisor;
  const unsigned long long frac = magnitude % divisor;

  // A value that rounds to zero must not render as "-0.00".
  const char* sign = (negative && magnitude != 0ULL) ? "-" : "";

  int written = -1;
  if (decimals == 1) {
    written = snprintf(out, outSize, "%s%llu.%01llu", sign, whole, frac);
  } else {
    written = snprintf(out, outSize, "%s%llu.%02llu", sign, whole, frac);
  }

  return written > 0 && static_cast<size_t>(written) < outSize;
}

// Renders a temperature field: the number, or the literal null when the
// reading is unavailable or fails the hard sanity guard.
void writeTemperatureField(char* out, size_t outSize, bool valid, float celsius,
                           int decimals) {
  // Defence in depth. The sensor layers already reject -127 (disconnected),
  // 85.0 (power-on reset) and out-of-range values, but a bug upstream must
  // never put a sentinel on the wire dressed up as a measurement.
  const bool serialisable = valid && isfinite(celsius) &&
                            celsius >= PACKET_HARD_MIN_C &&
                            celsius <= PACKET_HARD_MAX_C;

  if (!serialisable || !writeFixed(out, outSize, celsius, decimals)) {
    snprintf(out, outSize, "%s", JSON_NULL);
  }
}

}  // namespace

size_t formatMeasurementPacket(char* out, size_t outSize, const PacketInput& in) {
  if (out == nullptr || outSize == 0) {
    return 0;
  }
  out[0] = '\0';

  char skinField[FIELD_BUFFER_SIZE];
  char ambientField[FIELD_BUFFER_SIZE];
  char qualityField[FIELD_BUFFER_SIZE];

  writeTemperatureField(skinField, sizeof(skinField), in.skinValid,
                        in.skinTemperatureC, PACKET_SKIN_DECIMALS);
  writeTemperatureField(ambientField, sizeof(ambientField), in.ambientValid,
                        in.ambientTemperatureC, PACKET_AMBIENT_DECIMALS);

  // sensorQuality is never null and never NaN: an unusable input means "no
  // confidence", which is exactly 0.0.
  double quality = static_cast<double>(in.sensorQuality);
  if (!isfinite(quality) || quality < 0.0) {
    quality = 0.0;
  } else if (quality > 1.0) {
    quality = 1.0;
  }
  if (!writeFixed(qualityField, sizeof(qualityField), quality,
                  PACKET_QUALITY_DECIMALS)) {
    snprintf(qualityField, sizeof(qualityField), "0.00");
  }

  const int written = snprintf(
      out, outSize,
      "{\"timestampMs\":%lld,\"skinTemperatureC\":%s,"
      "\"ambientTemperatureC\":%s,\"sensorQuality\":%s}",
      static_cast<long long>(in.timestampMs), skinField, ambientField,
      qualityField);

  if (written <= 0 || static_cast<size_t>(written) >= outSize) {
    // Truncated: report failure rather than hand back half a JSON object.
    out[0] = '\0';
    return 0;
  }

  return static_cast<size_t>(written);
}

}  // namespace krebb
