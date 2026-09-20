#pragma once

#include <stddef.h>
#include <stdint.h>

// Krebb One BLE measurement packet serialisation.
//
// The wire format is fixed by the shared contract in docs/sensor-protocol.md
// and docs/BLE_PROTOCOL.md:
//
//   {"timestampMs":1760000000000,"skinTemperatureC":33.42,
//    "ambientTemperatureC":24.0,"sensorQuality":0.91}
//
// (one line, no spaces). Exactly these four keys, this casing, degrees
// Celsius. Any unavailable temperature is the JSON literal null - never a
// stale value, never a sentinel like -127.
//
// Pure C++: no Arduino headers, so it is unit-tested on the host.

namespace krebb {

struct PacketInput {
  // Unix milliseconds at the moment the packet is assembled. Always present.
  int64_t timestampMs = 0;

  // skinValid == false serialises skinTemperatureC as null.
  bool skinValid = false;
  float skinTemperatureC = 0.0f;

  // ambientValid == false serialises ambientTemperatureC as null.
  bool ambientValid = false;
  float ambientTemperatureC = 0.0f;

  // Always serialised as a number; clamped into [0.0, 1.0].
  float sensorQuality = 0.0f;
};

// Writes the packet as a NUL-terminated string into `out`.
// Returns the length in bytes excluding the NUL, or 0 if it did not fit
// (in which case `out` is left as an empty string). Never emits nan, inf,
// or a sensor sentinel value.
size_t formatMeasurementPacket(char* out, size_t outSize, const PacketInput& in);

}  // namespace krebb
