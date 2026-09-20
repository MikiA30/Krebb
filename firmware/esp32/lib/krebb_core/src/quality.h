#pragma once

#include <stdint.h>

#include "krebb_params.h"

// Krebb One sensorQuality estimator.
//
// Contact/measurement confidence in [0.0, 1.0], derived ONLY from DS18B20
// validity and recent temperature variation. It is deliberately a small
// heuristic, not a classifier, and ambient temperature is never an input.
//
// It expresses how much to trust the contact reading right now. It is not a
// medical or physiological score.
//
// Pure C++: no Arduino headers, so it is unit-tested on the host.

namespace krebb {

// Serial-log only. This never appears in the BLE packet.
enum class QualityReason : uint8_t {
  DISCONNECTED,  // latest read invalid / no sensor / no stable contact
  WARMING,       // settling after placement, not yet past the warmup window
  UNSTABLE,      // high short-window variation
  STABLE,        // sustained valid contact with low variation
};

const char* qualityReasonName(QualityReason reason);

class QualityEstimator {
 public:
  QualityEstimator() { reset(); }

  // Call once per 1 Hz tick with the result of the latest DS18B20 read.
  // An invalid read resets the window, the consecutive-valid counter and the
  // warmup, because contact was lost.
  void update(bool valid, float skinTemperatureC);

  void reset();

  float value() const { return value_; }
  QualityReason reason() const { return reason_; }

  // Components, exposed so the serial log can explain the score.
  float windowRangeC() const { return windowRangeC_; }
  float windowStdDevC() const { return windowStdDevC_; }
  uint32_t consecutiveValidS() const { return consecutiveValidS_; }
  uint8_t windowCount() const { return count_; }

 private:
  float samples_[params::QUALITY_WINDOW_S];
  uint8_t next_;   // ring-buffer write index
  uint8_t count_;  // samples held, saturating at QUALITY_WINDOW_S

  uint32_t consecutiveValidS_;
  float value_;
  float windowRangeC_;
  float windowStdDevC_;
  QualityReason reason_;
};

}  // namespace krebb
