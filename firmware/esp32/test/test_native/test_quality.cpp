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
}
