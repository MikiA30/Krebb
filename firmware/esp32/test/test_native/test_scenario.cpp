// Scripted end-to-end session simulation over the pure core.
//
// Walks a virtual 1 Hz session through warm-up, stable contact, a one-second
// contact-loss glitch, a full sensor unplug, recovery, and a DHT11 dropout,
// asserting the quality bands, the nulls, and above all that no stale reading
// is ever presented as live. Packets are printed so the trace can be eyeballed.
//
// No hardware, no Arduino: this is the regression net for the packet contract.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unity.h>

#include "freshness.h"
#include "krebb_params.h"
#include "packet.h"
#include "quality.h"
#include "suites.h"
#include "time_source.h"

namespace {

namespace P = krebb::params;

constexpr int64_t BUILD_EPOCH_MS = 1760000000000LL;

struct Sim {
  krebb::TimeSource time{BUILD_EPOCH_MS};
  krebb::QualityEstimator quality;
  krebb::AmbientSample ambient;  // last accepted DHT11 reading
  uint32_t second = 0;
  char packet[P::PACKET_BUFFER_SIZE] = {0};

  // Runs one 1 Hz tick and leaves the assembled packet in `packet`.
  void tick(bool skinValid, float skinC, bool ambientReadSucceeded, float ambientC) {
    const uint32_t uptimeMs = second * 1000u;

    // The DHT11 has its own 2 s cadence; it only refreshes on even seconds.
    if (ambientReadSucceeded && (second % 2 == 0)) {
      ambient.valid = true;
      ambient.celsius = ambientC;
      ambient.observedAtMs = uptimeMs;
    }

    quality.update(skinValid, skinC);

    const krebb::AmbientSelection amb =
        krebb::selectAmbientForPacket(ambient, uptimeMs, P::AMBIENT_MAX_AGE_MS);

    krebb::PacketInput in;
    in.timestampMs = time.nowUnixMs(uptimeMs);
    in.skinValid = skinValid;
    in.skinTemperatureC = skinC;
    in.ambientValid = amb.present;
    in.ambientTemperatureC = amb.celsius;
    in.sensorQuality = quality.value();

    const size_t len = krebb::formatMeasurementPacket(packet, sizeof(packet), in);
    TEST_ASSERT_TRUE(len > 0);
    TEST_ASSERT_TRUE(len < 128);

    ++second;
  }

  bool contains(const char* needle) const { return strstr(packet, needle) != nullptr; }
};

void printPacket(const Sim& sim, const char* phase) {
  printf("  [%3us] %-16s %s\n", sim.second - 1, phase, sim.packet);
}

void test_full_session_walkthrough() {
  Sim sim;
  printf("\n--- scripted session simulation -------------------------------\n");

  // -- phase 1: warm-up after placement, skin drifting up ---------------------
  // One short of the warmup threshold, so this phase is still WARMING.
  for (uint32_t i = 0; i < P::QUALITY_WARMUP_S - 1; ++i) {
    // Rises 32.00 -> ~33.40 over the warmup, i.e. real settling behaviour.
    const float skin = 32.00f + 1.40f * (static_cast<float>(i) /
                                         static_cast<float>(P::QUALITY_WARMUP_S));
    sim.tick(true, skin, true, 24.0f);
  }
  printPacket(sim, "warming");
  TEST_ASSERT_EQUAL_INT(static_cast<int>(krebb::QualityReason::WARMING),
                        static_cast<int>(sim.quality.reason()));
  TEST_ASSERT_TRUE(sim.quality.value() >= P::QUALITY_STABILIZING_MIN);
  TEST_ASSERT_TRUE(sim.quality.value() <= P::QUALITY_STABILIZING_MAX);
  TEST_ASSERT_TRUE(sim.contains("\"ambientTemperatureC\":24.0"));

  // -- phase 2: settled contact ----------------------------------------------
  for (uint32_t i = 0; i < 120; ++i) {
    sim.tick(true, 33.42f, true, 24.0f);
  }
  printPacket(sim, "stable");
  TEST_ASSERT_EQUAL_INT(static_cast<int>(krebb::QualityReason::STABLE),
                        static_cast<int>(sim.quality.reason()));
  TEST_ASSERT_TRUE(sim.quality.value() >= P::QUALITY_STABLE_MIN);
  TEST_ASSERT_TRUE(sim.contains("\"skinTemperatureC\":33.42"));

  // -- phase 3: one-second contact-loss glitch -------------------------------
  sim.tick(false, 0.0f, true, 24.0f);
  printPacket(sim, "glitch");
  TEST_ASSERT_TRUE(sim.contains("\"skinTemperatureC\":null"));
  TEST_ASSERT_TRUE(sim.contains("\"sensorQuality\":0.00"));
  // The previous good reading must NOT be repeated as if it were live.
  TEST_ASSERT_FALSE(sim.contains("33.42"));
  // Ambient is unaffected by a skin dropout.
  TEST_ASSERT_TRUE(sim.contains("\"ambientTemperatureC\":24.0"));

  // -- phase 4: contact returns, warmup restarts -----------------------------
  for (uint32_t i = 0; i < 15; ++i) {
    sim.tick(true, 33.40f, true, 24.0f);
  }
  printPacket(sim, "recovering");
  TEST_ASSERT_EQUAL_INT(static_cast<int>(krebb::QualityReason::WARMING),
                        static_cast<int>(sim.quality.reason()));
  TEST_ASSERT_TRUE(sim.quality.value() <= P::QUALITY_STABILIZING_MAX);
  TEST_ASSERT_TRUE(sim.contains("\"skinTemperatureC\":33.40"));

  // -- phase 5: sensor unplugged for 10 s ------------------------------------
  for (uint32_t i = 0; i < 10; ++i) {
    // The driver reports DEVICE_DISCONNECTED_C; it must never be serialised.
    sim.tick(false, P::SKIN_DISCONNECTED_C, true, 24.0f);
    TEST_ASSERT_TRUE(sim.contains("\"skinTemperatureC\":null"));
    TEST_ASSERT_TRUE(sim.contains("\"sensorQuality\":0.00"));
    TEST_ASSERT_FALSE(sim.contains("-127"));
  }
  printPacket(sim, "unplugged");

  // -- phase 6: replugged, climbs back through the bands ---------------------
  for (uint32_t i = 0; i < P::QUALITY_WARMUP_S; ++i) {
    sim.tick(true, 33.38f, true, 24.0f);
  }
  printPacket(sim, "recovered");
  TEST_ASSERT_EQUAL_INT(static_cast<int>(krebb::QualityReason::STABLE),
                        static_cast<int>(sim.quality.reason()));
  TEST_ASSERT_TRUE(sim.quality.value() >= P::QUALITY_STABLE_MIN);

  // -- phase 7: DHT11 stops answering ----------------------------------------
  // Within the freshness window the last valid ambient reading is still used.
  for (uint32_t i = 0; i < 4; ++i) {
    sim.tick(true, 33.38f, false, 0.0f);
  }
  printPacket(sim, "amb stale-ok");
  TEST_ASSERT_TRUE(sim.contains("\"ambientTemperatureC\":24.0"));

  // Past AMBIENT_MAX_AGE_MS it becomes null rather than a stale number.
  for (uint32_t i = 0; i < 6; ++i) {
    sim.tick(true, 33.38f, false, 0.0f);
  }
  printPacket(sim, "amb dropped");
  TEST_ASSERT_TRUE(sim.contains("\"ambientTemperatureC\":null"));
  // The skin channel is unaffected by an ambient failure.
  TEST_ASSERT_TRUE(sim.contains("\"skinTemperatureC\":33.38"));
  TEST_ASSERT_TRUE(sim.quality.value() >= P::QUALITY_STABLE_MIN);

  // -- phase 8: both sensors down -------------------------------------------
  sim.tick(false, P::SKIN_DISCONNECTED_C, false, 0.0f);
  printPacket(sim, "both down");
  TEST_ASSERT_TRUE(sim.contains("\"skinTemperatureC\":null"));
  TEST_ASSERT_TRUE(sim.contains("\"ambientTemperatureC\":null"));
  TEST_ASSERT_TRUE(sim.contains("\"sensorQuality\":0.00"));

  printf("---------------------------------------------------------------\n");
}

// Timestamps must advance by exactly one second per tick and never repeat.
void test_timestamps_advance_monotonically() {
  Sim sim;
  int64_t previous = -1;
  for (uint32_t i = 0; i < 30; ++i) {
    sim.tick(true, 33.0f, true, 24.0f);
    const char* field = strstr(sim.packet, "\"timestampMs\":");
    TEST_ASSERT_NOT_NULL(field);
    const long long value = atoll(field + strlen("\"timestampMs\":"));
    if (previous >= 0) {
      TEST_ASSERT_TRUE(value - previous == 1000);
    }
    previous = value;
  }
}

}  // namespace

void run_scenario_tests() {
  RUN_TEST(test_full_session_walkthrough);
  RUN_TEST(test_timestamps_advance_monotonically);
}
