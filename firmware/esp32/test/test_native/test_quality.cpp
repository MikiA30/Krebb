// sensorQuality heuristic tests.

#include <math.h>
#include <unity.h>

#include "krebb_params.h"
#include "quality.h"
#include "suites.h"

namespace {

using krebb::QualityEstimator;
using krebb::QualityReason;
namespace P = krebb::params;

// Feeds `seconds` of perfectly steady readings.
void feedSteady(QualityEstimator& q, uint32_t seconds, float celsius = 33.40f) {
  for (uint32_t i = 0; i < seconds; ++i) {
    q.update(true, celsius);
  }
}

void test_starts_disconnected() {
  QualityEstimator q;
  TEST_ASSERT_EQUAL_FLOAT(0.0f, q.value());
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::DISCONNECTED), static_cast<int>(q.reason()));
}

void test_invalid_read_is_zero_and_disconnected() {
  QualityEstimator q;
  feedSteady(q, 200);
  TEST_ASSERT_TRUE((q.value()) >= (P::QUALITY_STABLE_MIN));

  q.update(false, 0.0f);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, q.value());
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::DISCONNECTED), static_cast<int>(q.reason()));
  TEST_ASSERT_EQUAL_UINT32(0, q.consecutiveValidS());
}

void test_nan_reading_counts_as_invalid() {
  QualityEstimator q;
  feedSteady(q, 100);
  q.update(true, NAN);
  TEST_ASSERT_EQUAL_FLOAT(0.0f, q.value());
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::DISCONNECTED), static_cast<int>(q.reason()));
}

void test_warming_band_before_warmup_completes() {
  QualityEstimator q;
  feedSteady(q, 5);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::WARMING), static_cast<int>(q.reason()));
  TEST_ASSERT_TRUE((q.value()) >= (P::QUALITY_STABILIZING_MIN));
  TEST_ASSERT_TRUE((q.value()) <= (P::QUALITY_STABILIZING_MAX));
}

void test_warming_score_rises_towards_the_band_top() {
  QualityEstimator q1;
  feedSteady(q1, 5);
  const float early = q1.value();

  QualityEstimator q2;
  feedSteady(q2, P::QUALITY_WARMUP_S - 1);
  const float late = q2.value();

  TEST_ASSERT_TRUE((late) > (early));
  TEST_ASSERT_TRUE((late) <= (P::QUALITY_STABILIZING_MAX));
}

void test_stable_band_after_warmup() {
  QualityEstimator q;
  feedSteady(q, P::QUALITY_WARMUP_S);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::STABLE), static_cast<int>(q.reason()));
  TEST_ASSERT_TRUE((q.value()) >= (P::QUALITY_STABLE_MIN));
  TEST_ASSERT_TRUE((q.value()) <= (P::QUALITY_STABLE_MAX));
}

void test_long_stable_run_approaches_one() {
  QualityEstimator q;
  feedSteady(q, P::QUALITY_WARMUP_S + P::QUALITY_FULL_S);
  TEST_ASSERT_EQUAL_FLOAT(1.0f, q.value());
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::STABLE), static_cast<int>(q.reason()));
}

void test_high_variation_drops_to_stabilizing_band() {
  QualityEstimator q;
  feedSteady(q, P::QUALITY_WARMUP_S + 50);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::STABLE), static_cast<int>(q.reason()));

  // A swinging trace: range far above the unstable threshold.
  for (uint32_t i = 0; i < P::QUALITY_WINDOW_S; ++i) {
    q.update(true, (i % 2 == 0) ? 30.0f : 36.0f);
  }
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::UNSTABLE), static_cast<int>(q.reason()));
  TEST_ASSERT_TRUE((q.value()) >= (P::QUALITY_STABILIZING_MIN));
  TEST_ASSERT_TRUE((q.value()) <= (P::QUALITY_STABILIZING_MAX));
}

void test_invalid_read_resets_warmup() {
  QualityEstimator q;
  feedSteady(q, P::QUALITY_WARMUP_S + 30);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::STABLE), static_cast<int>(q.reason()));

  q.update(false, 0.0f);   // contact lost for one second
  q.update(true, 33.40f);  // and immediately back

  // Warmup restarts: a single good sample after a dropout is not "stable".
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::WARMING), static_cast<int>(q.reason()));
  TEST_ASSERT_EQUAL_UINT32(1, q.consecutiveValidS());
  TEST_ASSERT_TRUE((q.value()) <= (P::QUALITY_STABILIZING_MAX));
}

void test_window_is_discarded_across_a_dropout() {
  QualityEstimator q;
  feedSteady(q, 30, 33.0f);
  q.update(false, 0.0f);
  q.update(true, 40.0f);

  // The window holds only the post-dropout sample, so the old 33 C readings
  // cannot make the new contact look like a 7 C swing.
  TEST_ASSERT_EQUAL_UINT8(1, q.windowCount());
  TEST_ASSERT_EQUAL_FLOAT(0.0f, q.windowRangeC());
}

void test_components_are_reported() {
  QualityEstimator q;
  for (uint32_t i = 0; i < 10; ++i) {
    q.update(true, 33.00f + (i % 2 ? 0.04f : 0.0f));
  }
  TEST_ASSERT_EQUAL_UINT32(10, q.consecutiveValidS());
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.04f, q.windowRangeC());
  TEST_ASSERT_TRUE((q.windowStdDevC()) > (0.0f));
}

void test_value_always_within_zero_and_one() {
  QualityEstimator q;
  for (uint32_t i = 0; i < 500; ++i) {
    const bool valid = (i % 7) != 0;
    q.update(valid, 20.0f + static_cast<float>(i % 23));
    TEST_ASSERT_TRUE(isfinite(q.value()));
    TEST_ASSERT_TRUE((q.value()) >= (0.0f));
    TEST_ASSERT_TRUE((q.value()) <= (1.0f));
  }
}

void test_reason_names() {
  TEST_ASSERT_EQUAL_STRING("STABLE", krebb::qualityReasonName(QualityReason::STABLE));
  TEST_ASSERT_EQUAL_STRING("WARMING", krebb::qualityReasonName(QualityReason::WARMING));
  TEST_ASSERT_EQUAL_STRING("UNSTABLE", krebb::qualityReasonName(QualityReason::UNSTABLE));
  TEST_ASSERT_EQUAL_STRING("DISCONNECTED",
                           krebb::qualityReasonName(QualityReason::DISCONNECTED));
}

// --------------------------------------------------------------- hysteresis
//
// The bench log flipped STABLE (0.73-0.87) / UNSTABLE (0.45) second by second
// while the window range hovered around 0.6 C. These cover the latch.

// Fills the window with a trace of a given peak-to-peak range, centred on
// `centre`, so windowRangeC() lands on `rangeC` exactly.
void feedRange(QualityEstimator& q, uint32_t seconds, float rangeC,
               float centre = 33.40f) {
  for (uint32_t i = 0; i < seconds; ++i) {
    q.update(true, (i % 2 == 0) ? centre - rangeC * 0.5f : centre + rangeC * 0.5f);
  }
}

void test_near_threshold_oscillation_does_not_flap() {
  QualityEstimator q;
  feedSteady(q, P::QUALITY_WARMUP_S + 30);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::STABLE), static_cast<int>(q.reason()));

  // Cross the entry threshold once: now latched UNSTABLE.
  feedRange(q, P::QUALITY_WINDOW_S, 0.62f);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::UNSTABLE), static_cast<int>(q.reason()));

  // Now oscillate either side of it, the way the bench trace did. The range
  // never gets down to the 0.4 C exit bound, so the state must not move.
  for (int cycle = 0; cycle < 6; ++cycle) {
    feedRange(q, P::QUALITY_WINDOW_S, 0.56f);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::UNSTABLE),
                          static_cast<int>(q.reason()));
    feedRange(q, P::QUALITY_WINDOW_S, 0.62f);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::UNSTABLE),
                          static_cast<int>(q.reason()));
  }
}

void test_steady_ramp_goes_unstable() {
  QualityEstimator q;
  feedSteady(q, P::QUALITY_WARMUP_S + 30);

  // 0.1 C per second: after a full window the range is ~1.9 C, well past the
  // entry threshold.
  float c = 33.40f;
  for (uint32_t i = 0; i < P::QUALITY_WINDOW_S; ++i) {
    q.update(true, c);
    c += 0.1f;
  }
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::UNSTABLE), static_cast<int>(q.reason()));
  TEST_ASSERT_TRUE((q.windowRangeC()) > (P::QUALITY_RANGE_UNSTABLE_C));
}

void test_settling_goes_stable_only_after_the_hold() {
  QualityEstimator q;
  feedSteady(q, P::QUALITY_WARMUP_S + 30);
  feedRange(q, P::QUALITY_WINDOW_S, 0.62f);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::UNSTABLE), static_cast<int>(q.reason()));

  // Flush the swinging samples out of the window with steady ones. Only once
  // the window range itself is calm does the release clock start, so step
  // through one sample at a time and watch the streak.
  uint32_t calmSamples = 0;
  for (uint32_t i = 0; i < P::QUALITY_WINDOW_S * 2; ++i) {
    q.update(true, 33.40f);
    if (q.windowRangeC() <= P::QUALITY_UNSTABLE_EXIT_RANGE_C) {
      ++calmSamples;
    } else {
      calmSamples = 0;
    }

    if (calmSamples < P::QUALITY_UNSTABLE_EXIT_HOLD_S) {
      TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::UNSTABLE),
                            static_cast<int>(q.reason()));
    } else {
      TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::STABLE),
                            static_cast<int>(q.reason()));
    }
  }

  // It did settle in the end, rather than the loop simply never reaching it.
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::STABLE), static_cast<int>(q.reason()));
  TEST_ASSERT_TRUE((q.value()) >= (P::QUALITY_STABLE_MIN));
}

void test_deadband_range_does_not_count_towards_release() {
  QualityEstimator q;
  feedSteady(q, P::QUALITY_WARMUP_S + 30);
  feedRange(q, P::QUALITY_WINDOW_S, 0.62f);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::UNSTABLE), static_cast<int>(q.reason()));

  // 0.5 C sits between the 0.4 C exit bound and the 0.6 C entry threshold: it
  // neither re-triggers nor earns credit, however long it lasts.
  feedRange(q, P::QUALITY_WINDOW_S * 5, 0.50f);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::UNSTABLE), static_cast<int>(q.reason()));
  TEST_ASSERT_EQUAL_UINT32(0, q.calmStreakS());
}

void test_invalid_read_clears_the_latch() {
  QualityEstimator q;
  feedSteady(q, P::QUALITY_WARMUP_S + 30);
  feedRange(q, P::QUALITY_WINDOW_S, 1.20f);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::UNSTABLE), static_cast<int>(q.reason()));

  q.update(false, 0.0f);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::DISCONNECTED), static_cast<int>(q.reason()));
  TEST_ASSERT_EQUAL_UINT32(0, q.calmStreakS());

  // Fresh contact: the new window starts clean, so the old latch cannot make
  // the reconnected sensor look unstable. Warmup governs from here.
  feedSteady(q, 3);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(QualityReason::WARMING), static_cast<int>(q.reason()));
}

void test_hysteresis_thresholds_are_ordered() {
  // The exit bound must sit strictly below the entry threshold, or there is no
  // deadband and the latch buys nothing.
  TEST_ASSERT_TRUE((P::QUALITY_UNSTABLE_EXIT_RANGE_C) < (P::QUALITY_RANGE_UNSTABLE_C));
  TEST_ASSERT_TRUE((P::QUALITY_UNSTABLE_EXIT_HOLD_S) >= (1u));
}

}  // namespace

void run_quality_tests() {
  RUN_TEST(test_starts_disconnected);
  RUN_TEST(test_invalid_read_is_zero_and_disconnected);
  RUN_TEST(test_nan_reading_counts_as_invalid);
  RUN_TEST(test_warming_band_before_warmup_completes);
  RUN_TEST(test_warming_score_rises_towards_the_band_top);
  RUN_TEST(test_stable_band_after_warmup);
  RUN_TEST(test_long_stable_run_approaches_one);
  RUN_TEST(test_high_variation_drops_to_stabilizing_band);
  RUN_TEST(test_invalid_read_resets_warmup);
  RUN_TEST(test_window_is_discarded_across_a_dropout);
  RUN_TEST(test_components_are_reported);
  RUN_TEST(test_value_always_within_zero_and_one);
  RUN_TEST(test_reason_names);
  RUN_TEST(test_near_threshold_oscillation_does_not_flap);
  RUN_TEST(test_steady_ramp_goes_unstable);
  RUN_TEST(test_settling_goes_stable_only_after_the_hold);
  RUN_TEST(test_deadband_range_does_not_count_towards_release);
  RUN_TEST(test_invalid_read_clears_the_latch);
  RUN_TEST(test_hysteresis_thresholds_are_ordered);
}
