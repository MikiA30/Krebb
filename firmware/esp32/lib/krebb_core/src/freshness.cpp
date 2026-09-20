#include "freshness.h"

#include <math.h>

namespace krebb {

AmbientSelection selectAmbientForPacket(const AmbientSample& last, uint32_t nowMs,
                                        uint32_t maxAgeMs) {
  AmbientSelection out;

  if (!last.valid || !isfinite(last.celsius)) {
    // No successful read yet, or the stored value is unusable.
    out.present = false;
    out.ageMs = 0;
    return out;
  }

  // Unsigned subtraction in the millis() domain. This is deliberate: it stays
  // correct across the uint32 millis() rollover at ~49.7 days, where a signed
  // difference would go hugely negative and make a stale reading look fresh.
  //
  // A timestamp "in the future" (clock moved backwards, or a caller bug) also
  // lands here as a very large age, so it is treated as stale - the safe way
  // to be wrong.
  const uint32_t ageMs = nowMs - last.observedAtMs;

  out.ageMs = ageMs;
  out.celsius = last.celsius;
  out.present = (ageMs <= maxAgeMs);
  return out;
}

}  // namespace krebb
