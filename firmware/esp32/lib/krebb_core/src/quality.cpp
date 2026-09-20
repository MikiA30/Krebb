#include "quality.h"

#include <math.h>

namespace krebb {
namespace {

using namespace krebb::params;

// Maps a variation measure onto [0, 1]: at or below `good` scores 1.0, at or
// above `bad` scores 0.0, linear in between.
float variationScore(float measured, float good, float bad) {
  if (!(bad > good)) {
    return 0.0f;  // misconfigured thresholds: claim no confidence
  }
  if (measured <= good) {
    return 1.0f;
  }
  if (measured >= bad) {
    return 0.0f;
  }
  return (bad - measured) / (bad - good);
}

float clamp01(float v) {
  if (!isfinite(v)) {
    return 0.0f;
  }
  if (v < 0.0f) {
    return 0.0f;
  }
  if (v > 1.0f) {
    return 1.0f;
  }
  return v;
}

}  // namespace

const char* qualityReasonName(QualityReason reason) {
  switch (reason) {
    case QualityReason::DISCONNECTED: return "DISCONNECTED";
    case QualityReason::WARMING:      return "WARMING";
    case QualityReason::UNSTABLE:     return "UNSTABLE";
    case QualityReason::STABLE:       return "STABLE";
  }
  return "DISCONNECTED";
}

void QualityEstimator::reset() {
  next_ = 0;
  count_ = 0;
  consecutiveValidS_ = 0;
  unstableLatched_ = false;
  calmStreakS_ = 0;
  value_ = QUALITY_INVALID;
  windowRangeC_ = 0.0f;
  windowStdDevC_ = 0.0f;
  reason_ = QualityReason::DISCONNECTED;
}

void QualityEstimator::update(bool valid, float skinTemperatureC) {
  if (!valid || !isfinite(skinTemperatureC)) {
    // Contact was lost: the window describes a contact that no longer exists,
    // so it is discarded rather than averaged across the gap.
    reset();
    return;
  }

  samples_[next_] = skinTemperatureC;
  next_ = static_cast<uint8_t>((next_ + 1) % QUALITY_WINDOW_S);
  if (count_ < QUALITY_WINDOW_S) {
    ++count_;
  }
  if (consecutiveValidS_ < UINT32_MAX) {
    ++consecutiveValidS_;
  }

  float minC = samples_[0];
  float maxC = samples_[0];
  double sum = 0.0;
  for (uint8_t i = 0; i < count_; ++i) {
    const float s = samples_[i];
    if (s < minC) minC = s;
    if (s > maxC) maxC = s;
    sum += s;
  }
  const double mean = sum / static_cast<double>(count_);

  double varianceSum = 0.0;
  for (uint8_t i = 0; i < count_; ++i) {
    const double d = static_cast<double>(samples_[i]) - mean;
    varianceSum += d * d;
  }
  // Population stddev: this is a spread indicator, not an inference.
  windowStdDevC_ = static_cast<float>(sqrt(varianceSum / static_cast<double>(count_)));
  windowRangeC_ = maxC - minC;

  const float stability =
      fminf(variationScore(windowRangeC_, QUALITY_RANGE_STABLE_C, QUALITY_RANGE_UNSTABLE_C),
            variationScore(windowStdDevC_, QUALITY_STDDEV_STABLE_C, QUALITY_STDDEV_UNSTABLE_C));

  const bool warming = consecutiveValidS_ < QUALITY_WARMUP_S;

  // UNSTABLE is latched, with a tighter exit than entry. A window range that
  // hovers around QUALITY_RANGE_UNSTABLE_C (0.56 -> 0.62 -> 0.56) crosses the
  // entry threshold repeatedly, and without hysteresis the reported state -
  // and the quality band with it - flips every second or two. The app would
  // show that as flicker, which reads as a hardware fault rather than what it
  // is: a trace sitting right on the line.
  //
  // Entry is immediate, because a reading that has genuinely gone unsteady
  // should stop being called settled at once. Leaving needs the range to be
  // at or below QUALITY_UNSTABLE_EXIT_RANGE_C for QUALITY_UNSTABLE_EXIT_HOLD_S
  // consecutive 1 Hz samples: a real settle clears that easily, an oscillation
  // around the entry threshold never does.
  const bool variationHigh = (windowRangeC_ > QUALITY_RANGE_UNSTABLE_C) ||
                             (windowStdDevC_ > QUALITY_STDDEV_UNSTABLE_C);

  if (variationHigh) {
    unstableLatched_ = true;
    calmStreakS_ = 0;
  } else if (unstableLatched_) {
    if (windowRangeC_ <= QUALITY_UNSTABLE_EXIT_RANGE_C) {
      ++calmStreakS_;
      if (calmStreakS_ >= QUALITY_UNSTABLE_EXIT_HOLD_S) {
        unstableLatched_ = false;
        calmStreakS_ = 0;
      }
    } else {
      // Inside the deadband: not high enough to re-trigger, not calm enough to
      // count towards release. The streak restarts from zero.
      calmStreakS_ = 0;
    }
  }

  const bool unstable = unstableLatched_;

  if (warming || unstable) {
    const float warmProgress =
        clamp01(static_cast<float>(consecutiveValidS_) / static_cast<float>(QUALITY_WARMUP_S));
    // Half the band from how far through warmup we are, half from how steady
    // the trace already looks.
    const float blend = 0.5f * warmProgress + 0.5f * stability;
    value_ = QUALITY_STABILIZING_MIN +
             (QUALITY_STABILIZING_MAX - QUALITY_STABILIZING_MIN) * clamp01(blend);
    // Before warmup completes the reading cannot be called settled whatever
    // the variation looks like, so warmup is the more useful explanation.
    reason_ = warming ? QualityReason::WARMING : QualityReason::UNSTABLE;
  } else {
    const uint32_t settledS = consecutiveValidS_ - QUALITY_WARMUP_S;
    const float durationScore =
        clamp01(static_cast<float>(settledS) / static_cast<float>(QUALITY_FULL_S));
    const float blend = 0.5f * stability + 0.5f * durationScore;
    value_ = QUALITY_STABLE_MIN +
             (QUALITY_STABLE_MAX - QUALITY_STABLE_MIN) * clamp01(blend);
    reason_ = QualityReason::STABLE;
  }

  value_ = clamp01(value_);
}

}  // namespace krebb
