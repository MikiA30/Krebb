#pragma once

#include <stdint.h>

// Ambient-reading freshness rule for Krebb One.
//
// The DHT11 may not be sampled faster than about once per second (datasheet),
// and Krebb samples it every 2 s, while the BLE packet goes out at 1 Hz. So
// most packets necessarily repeat the previous ambient reading.
//
// The rule: carry the most recent VALID ambient reading only while it is
// younger than maxAgeMs; otherwise send null. This is the single bounded
// exception to "never send a prior reading as if it were live", and it exists
// only because the ambient cadence is slower than the packet cadence. The skin
// reading has no such exception - it is null the moment it is invalid.
//
// Pure C++: no Arduino headers, so it is unit-tested on the host.

namespace krebb {

// The last ambient reading the sensor layer accepted.
struct AmbientSample {
  bool valid = false;         // false until the first successful read
  float celsius = 0.0f;
  uint32_t observedAtMs = 0;  // millis() when this reading was taken
};

struct AmbientSelection {
  bool present = false;  // false -> ambientTemperatureC must be null
  float celsius = 0.0f;
  uint32_t ageMs = 0;    // age of the reading considered, for logging
};

// Decides whether `last` may go into the packet assembled at `nowMs`.
AmbientSelection selectAmbientForPacket(const AmbientSample& last, uint32_t nowMs,
                                        uint32_t maxAgeMs);

}  // namespace krebb
