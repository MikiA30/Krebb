// Packet serialisation tests: the BLE wire contract, byte for byte.

#include <math.h>
#include <string.h>
#include <unity.h>

#include "krebb_params.h"
#include "packet.h"
#include "suites.h"

namespace {

char buf[krebb::params::PACKET_BUFFER_SIZE];

size_t format(const krebb::PacketInput& in) {
  return krebb::formatMeasurementPacket(buf, sizeof(buf), in);
}

krebb::PacketInput baseInput() {
  krebb::PacketInput in;
  in.timestampMs = 1760000000000LL;
  in.skinValid = true;
  in.skinTemperatureC = 33.42f;
  in.ambientValid = true;
  in.ambientTemperatureC = 24.0f;
  in.sensorQuality = 0.91f;
  return in;
}

// --- the two examples fixed by the contract ---------------------------------

void test_exact_nominal_packet() {
  const size_t len = format(baseInput());
  TEST_ASSERT_EQUAL_STRING(
      "{\"timestampMs\":1760000000000,\"skinTemperatureC\":33.42,"
      "\"ambientTemperatureC\":24.0,\"sensorQuality\":0.91}",
      buf);
  TEST_ASSERT_TRUE(strlen(buf) == len);
}

void test_exact_unavailable_skin_packet() {
  krebb::PacketInput in = baseInput();
  in.timestampMs = 1760000001000LL;
  in.skinValid = false;
  in.ambientTemperatureC = 24.9f;
  in.sensorQuality = 0.0f;

  format(in);
  TEST_ASSERT_EQUAL_STRING(
      "{\"timestampMs\":1760000001000,\"skinTemperatureC\":null,"
      "\"ambientTemperatureC\":24.9,\"sensorQuality\":0.00}",
      buf);
}

// --- null handling ----------------------------------------------------------

void test_ambient_null() {
  krebb::PacketInput in = baseInput();
  in.ambientValid = false;
  format(in);
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"ambientTemperatureC\":null"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"skinTemperatureC\":33.42"));
}

void test_both_temperatures_null() {
  krebb::PacketInput in = baseInput();
  in.skinValid = false;
  in.ambientValid = false;
  in.sensorQuality = 0.0f;
  format(in);
  TEST_ASSERT_EQUAL_STRING(
      "{\"timestampMs\":1760000000000,\"skinTemperatureC\":null,"
      "\"ambientTemperatureC\":null,\"sensorQuality\":0.00}",
      buf);
}

// A value marked valid but carrying a sensor sentinel must still never reach
// the wire.
void test_disconnect_sentinel_never_serialised() {
  krebb::PacketInput in = baseInput();
  in.skinValid = true;
  in.skinTemperatureC = -127.0f;
  format(in);
  TEST_ASSERT_NULL(strstr(buf, "-127"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"skinTemperatureC\":null"));
}

// --- quality bounds ---------------------------------------------------------

void test_quality_boundaries() {
  krebb::PacketInput in = baseInput();

  in.sensorQuality = 0.0f;
  format(in);
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"sensorQuality\":0.00"));

  in.sensorQuality = 1.0f;
  format(in);
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"sensorQuality\":1.00"));
}

void test_quality_out_of_range_is_clamped() {
  krebb::PacketInput in = baseInput();

  in.sensorQuality = 1.4f;
  format(in);
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"sensorQuality\":1.00"));

  in.sensorQuality = -0.5f;
  format(in);
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"sensorQuality\":0.00"));
}

// --- non-finite inputs ------------------------------------------------------

void test_nan_and_inf_never_serialised() {
  krebb::PacketInput in = baseInput();
  in.skinTemperatureC = NAN;
  in.ambientTemperatureC = INFINITY;
  in.sensorQuality = NAN;
  format(in);

  TEST_ASSERT_NULL(strstr(buf, "nan"));
  TEST_ASSERT_NULL(strstr(buf, "NaN"));
  TEST_ASSERT_NULL(strstr(buf, "inf"));
  TEST_ASSERT_EQUAL_STRING(
      "{\"timestampMs\":1760000000000,\"skinTemperatureC\":null,"
      "\"ambientTemperatureC\":null,\"sensorQuality\":0.00}",
      buf);
}

// --- numeric formatting -----------------------------------------------------

void test_negative_temperatures() {
  krebb::PacketInput in = baseInput();
  in.skinTemperatureC = -12.25f;
  in.ambientTemperatureC = -3.5f;
  format(in);
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"skinTemperatureC\":-12.25"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"ambientTemperatureC\":-3.5"));
}

// A value that rounds to zero from below must not render as "-0.00".
void test_negative_zero_renders_without_sign() {
  krebb::PacketInput in = baseInput();
  in.skinTemperatureC = -0.001f;
  format(in);
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"skinTemperatureC\":0.00"));
  TEST_ASSERT_NULL(strstr(buf, "-0.00"));
}

void test_decimal_places_are_fixed() {
  krebb::PacketInput in = baseInput();
  in.skinTemperatureC = 30.0f;
  in.ambientTemperatureC = 20.0f;
  in.sensorQuality = 0.5f;
  format(in);
  TEST_ASSERT_EQUAL_STRING(
      "{\"timestampMs\":1760000000000,\"skinTemperatureC\":30.00,"
      "\"ambientTemperatureC\":20.0,\"sensorQuality\":0.50}",
      buf);
}

void test_rounding_is_half_away_from_zero() {
  krebb::PacketInput in = baseInput();
  in.skinTemperatureC = 33.4567f;
  in.ambientTemperatureC = 24.44f;
  format(in);
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"skinTemperatureC\":33.46"));
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"ambientTemperatureC\":24.4"));
}

// --- 64-bit timestamps ------------------------------------------------------

void test_64_bit_timestamp() {
  krebb::PacketInput in = baseInput();
  // Well past the 32-bit seconds range; ~1.76e12 ms is "now" in Unix ms.
  in.timestampMs = 1760000000123LL;
  format(in);
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"timestampMs\":1760000000123"));

  in.timestampMs = 4102444800000LL;  // year 2100
  format(in);
  TEST_ASSERT_NOT_NULL(strstr(buf, "\"timestampMs\":4102444800000"));
}

// --- buffer safety ----------------------------------------------------------

void test_packet_fits_well_under_buffer() {
  krebb::PacketInput in = baseInput();
  in.timestampMs = 4102444800000LL;
  in.skinTemperatureC = -12.25f;
  in.ambientTemperatureC = -3.5f;
  const size_t len = format(in);
  TEST_ASSERT_TRUE(len > 0);
  // Headroom matters: the notify path compares this against (MTU - 3).
  TEST_ASSERT_TRUE(len < 128);
}

void test_too_small_buffer_fails_cleanly() {
  char tiny[16];
  const size_t len =
      krebb::formatMeasurementPacket(tiny, sizeof(tiny), baseInput());
  // No truncated half-JSON is ever handed back.
  TEST_ASSERT_TRUE(len == 0);
  TEST_ASSERT_EQUAL_STRING("", tiny);
}

void test_null_buffer_is_rejected() {
  TEST_ASSERT_TRUE(krebb::formatMeasurementPacket(nullptr, 64, baseInput()) == 0);
}

// --- no extra fields --------------------------------------------------------

void test_exactly_four_keys_and_no_humidity() {
  format(baseInput());
  int colons = 0;
  for (const char* p = buf; *p != '\0'; ++p) {
    if (*p == ':') ++colons;
  }
  TEST_ASSERT_EQUAL_INT(4, colons);
  TEST_ASSERT_NULL(strstr(buf, "umidity"));
}

}  // namespace

void run_packet_tests() {
  RUN_TEST(test_exact_nominal_packet);
  RUN_TEST(test_exact_unavailable_skin_packet);
  RUN_TEST(test_ambient_null);
  RUN_TEST(test_both_temperatures_null);
  RUN_TEST(test_disconnect_sentinel_never_serialised);
  RUN_TEST(test_quality_boundaries);
  RUN_TEST(test_quality_out_of_range_is_clamped);
  RUN_TEST(test_nan_and_inf_never_serialised);
  RUN_TEST(test_negative_temperatures);
  RUN_TEST(test_negative_zero_renders_without_sign);
  RUN_TEST(test_decimal_places_are_fixed);
  RUN_TEST(test_rounding_is_half_away_from_zero);
  RUN_TEST(test_64_bit_timestamp);
  RUN_TEST(test_packet_fits_well_under_buffer);
  RUN_TEST(test_too_small_buffer_fails_cleanly);
  RUN_TEST(test_null_buffer_is_rejected);
  RUN_TEST(test_exactly_four_keys_and_no_humidity);
}
