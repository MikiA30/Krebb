#pragma once

#include <stdint.h>

// Krebb One clock.
//
// The BLE contract requires timestampMs to be Unix milliseconds and never
// null, but this board has no RTC, no battery-backed clock, and no NTP, and
// the shared protocol defines no time-sync mechanism. Adding one would be a
// contract change, so it is NOT done here - see README.md "Time and
// timestampMs" and the open question raised with the software lead.
//
// Until an epoch is supplied at runtime (serial `t <unix_ms>`), this falls
// back to BUILD_UNIX_EPOCH_S * 1000 + uptime, which is only as correct as
// "when this firmware was compiled". The firmware reports
// TIME=UNSYNCED(dev-fallback) for as long as that is the case, so nobody
// mistakes a dev-fallback timestamp for a real one.
//
// Pure C++: no Arduino headers, so it is unit-tested on the host.

namespace krebb {

class TimeSource {
 public:
  // buildEpochMs is the dev-only fallback origin (BUILD_UNIX_EPOCH_S * 1000).
  explicit TimeSource(int64_t buildEpochMs);

  // Unix milliseconds. `uptimeMs` is millis(); call it monotonically, as the
  // rollover counter is advanced by observing the value go backwards.
  int64_t nowUnixMs(uint32_t uptimeMs);

  // Anchors the clock: from now on nowUnixMs() returns unixMs at this uptime.
  void setEpochMs(int64_t unixMs, uint32_t uptimeMs);

  bool isSynced() const { return synced_; }

 private:
  // Total elapsed ms since boot, rollover-corrected.
  int64_t elapsedMs(uint32_t uptimeMs) const;

  int64_t epochOffsetMs_;  // Unix ms at elapsed == 0
  uint32_t lastUptimeMs_;
  uint32_t wrapCount_;
  bool synced_;
};

}  // namespace krebb
