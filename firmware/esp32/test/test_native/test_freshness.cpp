// Ambient freshness rule tests.

#include <unity.h>

#include "freshness.h"
#include "krebb_params.h"
#include "suites.h"

namespace {

using krebb::AmbientSample;
using krebb::AmbientSelection;
namespace P = krebb::params;

AmbientSample validAt(uint32_t atMs, float celsius = 24.9f) {
  AmbientSample s;
  s.valid = true;
  s.celsius = celsius;
  s.observedAtMs = atMs;
  return s;
}

void test_no_reading_yet_is_absent() {
  AmbientSample none;  // valid == false
  const AmbientSelection out =
      krebb::selectAmbientForPacket(none, 10000, P::AMBIENT_MAX_AGE_MS);
  TEST_ASSERT_FALSE(out.present);
}

void test_fresh_reading_is_used() {
  const AmbientSelection out =
      krebb::selectAmbientForPacket(validAt(10000), 11000, P::AMBIENT_MAX_AGE_MS);
  TEST_ASSERT_TRUE(out.present);
  TEST_ASSERT_EQUAL_FLOAT(24.9f, out.celsius);
  TEST_ASSERT_EQUAL_UINT32(1000, out.ageMs);
}

void test_reading_exactly_at_max_age_is_still_used() {
  const AmbientSelection out = krebb::selectAmbientForPacket(
      validAt(1000), 1000 + P::AMBIENT_MAX_AGE_MS, P::AMBIENT_MAX_AGE_MS);
  TEST_ASSERT_TRUE(out.present);
}

void test_reading_past_max_age_becomes_null() {
  const AmbientSelection out = krebb::selectAmbientForPacket(
      validAt(1000), 1000 + P::AMBIENT_MAX_AGE_MS + 1, P::AMBIENT_MAX_AGE_MS);
  TEST_ASSERT_FALSE(out.present);
  TEST_ASSERT_EQUAL_UINT32(P::AMBIENT_MAX_AGE_MS + 1, out.ageMs);
}

// A DHT11 sampled every 2 s must survive the gaps between samples, which is
// the whole reason the rule exists.
void test_normal_two_second_cadence_stays_fresh() {
  const AmbientSample s = validAt(0);
  for (uint32_t now = 0; now <= 2000; now += 500) {
    TEST_ASSERT_TRUE(krebb::selectAmbientForPacket(s, now, P::AMBIENT_MAX_AGE_MS).present);
  }
}

// millis() rolls over every ~49.7 days; unsigned subtraction keeps the age
// correct across the wrap instead of making a stale reading look fresh.
void test_millis_rollover_is_handled() {
  const uint32_t beforeWrap = 0xFFFFFF00u;
  const uint32_t afterWrap = 0x00000300u;  // 0x400 == 1024 ms later, wrapped

  const AmbientSelection out = krebb::selectAmbientForPacket(
      validAt(beforeWrap), afterWrap, P::AMBIENT_MAX_AGE_MS);
  TEST_ASSERT_EQUAL_UINT32(1024u, out.ageMs);
  TEST_ASSERT_TRUE(out.present);
}

// A timestamp "in the future" must fail safe (treated as stale), not be
// accepted as a fresh reading.
void test_future_timestamp_is_treated_as_stale() {
  const AmbientSelection out =
      krebb::selectAmbientForPacket(validAt(50000), 40000, P::AMBIENT_MAX_AGE_MS);
  TEST_ASSERT_FALSE(out.present);
}

void test_zero_max_age_only_accepts_same_millisecond() {
  TEST_ASSERT_TRUE(krebb::selectAmbientForPacket(validAt(7000), 7000, 0).present);
  TEST_ASSERT_FALSE(krebb::selectAmbientForPacket(validAt(7000), 7001, 0).present);
}

}  // namespace

void run_freshness_tests() {
  RUN_TEST(test_no_reading_yet_is_absent);
  RUN_TEST(test_fresh_reading_is_used);
  RUN_TEST(test_reading_exactly_at_max_age_is_still_used);
  RUN_TEST(test_reading_past_max_age_becomes_null);
  RUN_TEST(test_normal_two_second_cadence_stays_fresh);
  RUN_TEST(test_millis_rollover_is_handled);
  RUN_TEST(test_future_timestamp_is_treated_as_stale);
  RUN_TEST(test_zero_max_age_only_accepts_same_millisecond);
}
