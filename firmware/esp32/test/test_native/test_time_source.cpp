// Clock tests: 64-bit Unix milliseconds, dev fallback, and sync behaviour.

#include <unity.h>

#include "suites.h"
#include "time_source.h"

namespace {

// 2025-10-09T07:33:20Z, i.e. the ~1.76e12 ms range the contract examples use.
constexpr int64_t BUILD_EPOCH_MS = 1760000000000LL;

void test_unsynced_uses_build_epoch_fallback() {
  krebb::TimeSource t(BUILD_EPOCH_MS);
  TEST_ASSERT_FALSE(t.isSynced());
  TEST_ASSERT_TRUE(BUILD_EPOCH_MS + 1234 == t.nowUnixMs(1234));
}

void test_time_advances_with_uptime() {
  krebb::TimeSource t(BUILD_EPOCH_MS);
  const int64_t a = t.nowUnixMs(1000);
  const int64_t b = t.nowUnixMs(2500);
  TEST_ASSERT_TRUE((b - a) == 1500);
}

void test_set_epoch_marks_synced_and_anchors_now() {
  krebb::TimeSource t(BUILD_EPOCH_MS);
  t.nowUnixMs(5000);

  const int64_t real = 1760000123456LL;
  t.setEpochMs(real, 5000);

  TEST_ASSERT_TRUE(t.isSynced());
  TEST_ASSERT_TRUE(real == t.nowUnixMs(5000));
  // One second of uptime later, one second of wall clock later.
  TEST_ASSERT_TRUE((real + 1000) == t.nowUnixMs(6000));
}

void test_sync_survives_large_64_bit_values() {
  krebb::TimeSource t(BUILD_EPOCH_MS);
  const int64_t year2100 = 4102444800000LL;
  t.setEpochMs(year2100, 10000);
  TEST_ASSERT_TRUE(year2100 == t.nowUnixMs(10000));
  TEST_ASSERT_TRUE((year2100 + 60000) == t.nowUnixMs(70000));
}

void test_value_is_not_truncated_to_32_bits() {
  krebb::TimeSource t(BUILD_EPOCH_MS);
  const int64_t now = t.nowUnixMs(0);
  // 1.76e12 does not fit in int32; a truncating implementation would wrap.
  TEST_ASSERT_TRUE(now > 4294967296LL);
  TEST_ASSERT_TRUE(now == BUILD_EPOCH_MS);
}

// millis() wraps every ~49.7 days. A session should never see this, but a
// timestamp that silently jumped 49 days backwards would corrupt a recording.
void test_millis_rollover_keeps_time_moving_forward() {
  krebb::TimeSource t(BUILD_EPOCH_MS);
  const int64_t before = t.nowUnixMs(0xFFFFFF00u);
  const int64_t after = t.nowUnixMs(0x00000100u);  // wrapped, 0x200 ms later
  TEST_ASSERT_TRUE(after > before);
  TEST_ASSERT_TRUE((after - before) == 0x200);
}

void test_sync_after_rollover_stays_consistent() {
  krebb::TimeSource t(BUILD_EPOCH_MS);
  t.nowUnixMs(0xFFFFFF00u);
  const int64_t real = 1760000555000LL;
  t.setEpochMs(real, 0x00000100u);
  TEST_ASSERT_TRUE(real == t.nowUnixMs(0x00000100u));
  // 0x100 + 1000 ms of uptime -> exactly 1000 ms of wall clock.
  TEST_ASSERT_TRUE((real + 1000) == t.nowUnixMs(0x00000100u + 1000u));
}

}  // namespace

void run_time_source_tests() {
  RUN_TEST(test_unsynced_uses_build_epoch_fallback);
  RUN_TEST(test_time_advances_with_uptime);
  RUN_TEST(test_set_epoch_marks_synced_and_anchors_now);
  RUN_TEST(test_sync_survives_large_64_bit_values);
  RUN_TEST(test_value_is_not_truncated_to_32_bits);
  RUN_TEST(test_millis_rollover_keeps_time_moving_forward);
  RUN_TEST(test_sync_after_rollover_stays_consistent);
}
