#include "time_source.h"

namespace krebb {
namespace {
// millis() is uint32 and rolls over every 2^32 ms (~49.7 days).
constexpr int64_t MILLIS_ROLLOVER = 4294967296LL;
}  // namespace

TimeSource::TimeSource(int64_t buildEpochMs)
    : epochOffsetMs_(buildEpochMs),
      lastUptimeMs_(0),
      wrapCount_(0),
      synced_(false) {}

int64_t TimeSource::elapsedMs(uint32_t uptimeMs) const {
  return static_cast<int64_t>(wrapCount_) * MILLIS_ROLLOVER +
         static_cast<int64_t>(uptimeMs);
}

int64_t TimeSource::nowUnixMs(uint32_t uptimeMs) {
  // A value lower than the previous one means millis() rolled over. A single
  // session is not expected to run 49.7 days, but a timestamp that silently
  // jumps back 49 days would corrupt a recorded session, so it is handled.
  if (uptimeMs < lastUptimeMs_) {
    ++wrapCount_;
  }
  lastUptimeMs_ = uptimeMs;

  return epochOffsetMs_ + elapsedMs(uptimeMs);
}

void TimeSource::setEpochMs(int64_t unixMs, uint32_t uptimeMs) {
  if (uptimeMs < lastUptimeMs_) {
    ++wrapCount_;
  }
  lastUptimeMs_ = uptimeMs;

  // Anchor so that nowUnixMs(uptimeMs) == unixMs from here on.
  epochOffsetMs_ = unixMs - elapsedMs(uptimeMs);
  synced_ = true;
}

}  // namespace krebb
