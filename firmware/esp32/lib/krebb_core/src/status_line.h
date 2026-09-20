#pragma once

#include <stddef.h>
#include <stdint.h>

// Krebb One serial status line.
//
// Serial diagnostics only. Nothing here appears in the BLE packet, and the
// packet contract does not depend on any of it.
//
// It was one long snprintf() whose separators were written into the format
// string by hand, which made a missing " | " a silent, state-dependent
// cosmetic bug. Here the groups are rendered individually and joined by a
// single function that always writes exactly " | " between them, so a
// separator cannot go missing whatever the field contents are - and, being
// pure C++, the exact bytes are asserted on the host.

namespace krebb {

struct StatusLineInput {
  uint32_t uptimeS = 0;

  bool skinValid = false;
  float skinCelsius = 0.0f;
  bool skinDevicePresent = false;

  // Result of the freshness rule: false means the packet carried null.
  bool ambientPresent = false;
  float ambientCelsius = 0.0f;
  uint32_t ambientAgeMs = 0;

  // Serial-only debug value, and only meaningful alongside a fresh ambient
  // reading: it came out of the same DHT11 frame. When the ambient reading is
  // absent or stale the humidity beside it is too, and the line prints "--"
  // rather than a number that looks like a live measurement.
  bool humidityPresent = false;
  float humidityPercent = 0.0f;

  float quality = 0.0f;
  const char* qualityReason = "";
  float windowRangeC = 0.0f;
  uint32_t consecutiveValidS = 0;

  const char* bleState = "";
  uint16_t mtu = 0;
  bool timeSynced = false;
};

// Writes the NUL-terminated status line into `out`. Returns its length
// excluding the NUL, or 0 if `out` was too small to hold the whole line.
size_t formatStatusLine(char* out, size_t outSize, const StatusLineInput& in);

}  // namespace krebb
