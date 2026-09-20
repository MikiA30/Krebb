// DHT11 sample validation tests.
//
// The case that matters most is the all-zero frame: it passes the DHT11's own
// checksum, so nothing downstream would question a fabricated "0.0 degC" until
// the freshness rule aged it out five seconds later.

#include <math.h>
#include <unity.h>

#include "ambient_validate.h"
#include "krebb_params.h"
#include "suites.h"

namespace {

using krebb::AmbientReject;
using krebb::validateAmbientSample;
namespace P = krebb::params;

void expectRejected(float temperatureC, float humidityPercent, AmbientReject why) {
  const krebb::AmbientValidation v = validateAmbientSample(temperatureC, humidityPercent);
  TEST_ASSERT_FALSE(v.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(why), static_cast<int>(v.reject));
}

void expectAccepted(float temperatureC, float humidityPercent) {
  const krebb::AmbientValidation v = validateAmbientSample(temperatureC, humidityPercent);
  TEST_ASSERT_TRUE(v.valid);
  TEST_ASSERT_EQUAL_INT(static_cast<int>(AmbientReject::NONE), static_cast<int>(v.reject));
}

void test_all_zero_frame_is_rejected() {
  expectRejected(0.0f, 0.0f, AmbientReject::ALL_ZERO_FRAME);
}

void test_zero_humidity_is_rejected() {
  // Non-zero temperature, so this is a humidity-range failure rather than the
  // all-zero frame.
  expectRejected(22.0f, 0.0f, AmbientReject::HUMIDITY_RANGE);
}

void test_humidity_just_below_range_is_rejected() {
  expectRejected(25.0f, 19.0f, AmbientReject::HUMIDITY_RANGE);
}

void test_humidity_just_above_range_is_rejected() {
  expectRejected(25.0f, 91.0f, AmbientReject::HUMIDITY_RANGE);
}

void test_humidity_at_the_range_edges_is_accepted() {
  expectAccepted(25.0f, P::AMBIENT_HUMIDITY_MIN_PCT);  // 20 %RH
  expectAccepted(25.0f, P::AMBIENT_HUMIDITY_MAX_PCT);  // 90 %RH
}

void test_ordinary_room_reading_is_accepted() {
  expectAccepted(25.3f, 21.0f);
}

void test_nan_temperature_is_rejected() {
  expectRejected(NAN, 45.0f, AmbientReject::NOT_FINITE);
}

void test_nan_humidity_is_rejected() {
  expectRejected(25.0f, NAN, AmbientReject::NOT_FINITE);
}

void test_temperature_outside_the_operating_range_is_rejected() {
  expectRejected(-0.5f, 45.0f, AmbientReject::TEMPERATURE_RANGE);
  expectRejected(50.5f, 45.0f, AmbientReject::TEMPERATURE_RANGE);
}

void test_temperature_at_the_range_edges_is_accepted() {
  // 0 degC is a legitimate reading; only 0 degC WITH 0 %RH is the bad frame.
  expectAccepted(P::AMBIENT_SANITY_MIN_C, 45.0f);
  expectAccepted(P::AMBIENT_SANITY_MAX_C, 45.0f);
}

void test_reject_messages_are_specific() {
  // The log must name the wiring fault, not blame the humidity range for it.
  TEST_ASSERT_EQUAL_STRING(
      "DHT11 all-zero frame (0.0C / 0%RH) - checksum-valid but not a measurement",
      krebb::ambientRejectMessage(AmbientReject::ALL_ZERO_FRAME));
  TEST_ASSERT_EQUAL_STRING("", krebb::ambientRejectMessage(AmbientReject::NONE));
}

}  // namespace

void run_ambient_validate_tests() {
  RUN_TEST(test_all_zero_frame_is_rejected);
  RUN_TEST(test_zero_humidity_is_rejected);
  RUN_TEST(test_humidity_just_below_range_is_rejected);
  RUN_TEST(test_humidity_just_above_range_is_rejected);
  RUN_TEST(test_humidity_at_the_range_edges_is_accepted);
  RUN_TEST(test_ordinary_room_reading_is_accepted);
  RUN_TEST(test_nan_temperature_is_rejected);
  RUN_TEST(test_nan_humidity_is_rejected);
  RUN_TEST(test_temperature_outside_the_operating_range_is_rejected);
  RUN_TEST(test_temperature_at_the_range_edges_is_accepted);
  RUN_TEST(test_reject_messages_are_specific);
}
