// Serial status line formatting tests.
//
// The line is diagnostics, not contract, but a bench session is read off it,
// so a separator that disappears depending on the BLE state string costs real
// debugging time. These assert the exact bytes.

#include <string.h>
#include <unity.h>

#include "status_line.h"
#include "suites.h"

namespace {

using krebb::StatusLineInput;

// A fully-populated line: valid skin, fresh ambient, subscribed client.
StatusLineInput nominal() {
  StatusLineInput in;
  in.uptimeS = 123;
  in.skinValid = true;
  in.skinCelsius = 33.42f;
  in.skinDevicePresent = true;
  in.ambientPresent = true;
  in.ambientCelsius = 24.0f;
  in.ambientAgeMs = 1200;
  in.humidityPresent = true;
  in.humidityPercent = 45.0f;
  in.quality = 0.91f;
  in.qualityReason = "STABLE";
  in.windowRangeC = 0.08f;
  in.consecutiveValidS = 240;
  in.bleState = "SUBSCRIBED";
  in.mtu = 247;
  in.timeSynced = true;
  return in;
}

void format(const StatusLineInput& in, char* out, size_t outSize) {
  const size_t length = krebb::formatStatusLine(out, outSize, in);
  TEST_ASSERT_EQUAL_size_t(strlen(out), length);
}

void test_nominal_line_is_exact() {
  char out[256];
  format(nominal(), out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING(
      "[00123s] skin=33.42C ok | amb=24.0C age=1.2s hum=45% | "
      "q=0.91 STABLE win_range=0.08C consec=240s | "
      "ble=SUBSCRIBED mtu=247 | TIME=SYNCED",
      out);
}

// The reported bug: "ble=ADVERTISINGmtu=0" and "mtu=0 |TIME=...". Both
// separators are checked against the state strings that showed it.
void test_separators_survive_the_advertising_state() {
  StatusLineInput in = nominal();
  in.bleState = "ADVERTISING";
  in.mtu = 0;
  in.timeSynced = false;

  char out[256];
  format(in, out, sizeof(out));
  TEST_ASSERT_NOT_NULL(strstr(out, "ble=ADVERTISING mtu=0"));
  TEST_ASSERT_NOT_NULL(strstr(out, "mtu=0 | TIME=UNSYNCED(dev-fallback)"));
  TEST_ASSERT_NULL(strstr(out, "ADVERTISINGmtu"));
  TEST_ASSERT_NULL(strstr(out, " |TIME"));
}

void test_unstable_reason_does_not_eat_a_separator() {
  StatusLineInput in = nominal();
  in.qualityReason = "UNSTABLE";
  in.quality = 0.45f;
  in.windowRangeC = 0.62f;
  in.bleState = "CONNECTED";
  in.mtu = 0;

  char out[256];
  format(in, out, sizeof(out));
  TEST_ASSERT_EQUAL_STRING(
      "[00123s] skin=33.42C ok | amb=24.0C age=1.2s hum=45% | "
      "q=0.45 UNSTABLE win_range=0.62C consec=240s | "
      "ble=CONNECTED mtu=0 | TIME=SYNCED",
      out);
}

// Five groups follow the uptime prefix, so every state combination keeps
// exactly four " | " separators - no more, and crucially no fewer.
void test_separator_count_is_constant_across_states() {
  const char* states[] = {"ADVERTISING", "CONNECTED", "SUBSCRIBED"};
  const char* reasons[] = {"DISCONNECTED", "WARMING", "UNSTABLE", "STABLE"};

  for (int s = 0; s < 3; ++s) {
    for (int r = 0; r < 4; ++r) {
      for (int skin = 0; skin < 2; ++skin) {
        for (int amb = 0; amb < 2; ++amb) {
          StatusLineInput in = nominal();
          in.bleState = states[s];
          in.qualityReason = reasons[r];
          in.skinValid = (skin == 1);
          in.ambientPresent = (amb == 1);

          char out[256];
          format(in, out, sizeof(out));

          int separators = 0;
          for (const char* p = strstr(out, " | "); p != nullptr;
               p = strstr(p + 3, " | ")) {
            ++separators;
          }
          TEST_ASSERT_EQUAL_INT(4, separators);
        }
      }
    }
  }
}

void test_stale_ambient_hides_the_humidity() {
  StatusLineInput in = nominal();
  in.ambientPresent = false;
  in.ambientAgeMs = 7400;

  char out[256];
  format(in, out, sizeof(out));
  // Not "hum=0%", and not the last reading either: with no fresh frame there
  // is no fresh humidity.
  TEST_ASSERT_NOT_NULL(strstr(out, "amb=null age=7.4s hum=--"));
  TEST_ASSERT_NULL(strstr(out, "hum=45%"));
  TEST_ASSERT_NULL(strstr(out, "hum=0%"));
}

void test_missing_humidity_prints_the_placeholder() {
  StatusLineInput in = nominal();
  in.humidityPresent = false;
  in.humidityPercent = 0.0f;

  char out[256];
  format(in, out, sizeof(out));
  TEST_ASSERT_NOT_NULL(strstr(out, "hum=--"));
  TEST_ASSERT_NULL(strstr(out, "hum=0%"));
}

void test_disconnected_skin_reports_no_device() {
  StatusLineInput in = nominal();
  in.skinValid = false;
  in.skinDevicePresent = false;

  char out[256];
  format(in, out, sizeof(out));
  TEST_ASSERT_NOT_NULL(strstr(out, "skin=null NO_DEVICE | amb="));
}

void test_present_but_failing_skin_reports_err() {
  StatusLineInput in = nominal();
  in.skinValid = false;
  in.skinDevicePresent = true;

  char out[256];
  format(in, out, sizeof(out));
  TEST_ASSERT_NOT_NULL(strstr(out, "skin=null ERR | amb="));
}

// A buffer too small yields an empty string, never a half-joined line.
void test_short_buffer_yields_nothing() {
  char out[40];
  TEST_ASSERT_EQUAL_size_t(0, krebb::formatStatusLine(out, sizeof(out), nominal()));
  TEST_ASSERT_EQUAL_STRING("", out);
}

void test_zero_sized_buffer_is_safe() {
  char out[1] = {'x'};
  TEST_ASSERT_EQUAL_size_t(0, krebb::formatStatusLine(out, 0, nominal()));
}

}  // namespace

void run_status_line_tests() {
  RUN_TEST(test_nominal_line_is_exact);
  RUN_TEST(test_separators_survive_the_advertising_state);
  RUN_TEST(test_unstable_reason_does_not_eat_a_separator);
  RUN_TEST(test_separator_count_is_constant_across_states);
  RUN_TEST(test_stale_ambient_hides_the_humidity);
  RUN_TEST(test_missing_humidity_prints_the_placeholder);
  RUN_TEST(test_disconnected_skin_reports_no_device);
  RUN_TEST(test_present_but_failing_skin_reports_err);
  RUN_TEST(test_short_buffer_yields_nothing);
  RUN_TEST(test_zero_sized_buffer_is_safe);
}
