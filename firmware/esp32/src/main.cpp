// Krebb One - ESP32-S3 firmware (Milestones 1 and 2)
//
// Reads a DS18B20 skin-contact temperature and a DHT11 ambient temperature,
// and streams one UTF-8 JSON packet per second over BLE to the iOS app.
//
// Krebb One is a research prototype. It is NOT a medical device, calorimeter,
// or validated calorie tracker. The DS18B20 reports a local skin-contact
// temperature trend - not a clinical body temperature, and nothing about
// calories or metabolic rate. Only the within-session change from baseline
// carries meaning. Ambient temperature is confounder context, never a
// correction factor.
//
// Wire contract: docs/sensor-protocol.md, docs/BLE_PROTOCOL.md
// Pin contract:  hardware/wiring/pin-map.md (mirrored in include/pins.h)

#include <Arduino.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "ambient_sensor.h"
#include "ble_service.h"
#include "freshness.h"
#include "krebb_params.h"
#include "packet.h"
#include "pins.h"
#include "quality.h"
#include "skin_sensor.h"
#include "time_source.h"

namespace {

using namespace krebb::params;

constexpr const char* FIRMWARE_VERSION = "0.1.0";

// Injected by scripts/inject_build_epoch.py. The fallback keeps a hand-run
// compile working; it only ever affects the dev-only clock fallback.
#ifndef BUILD_UNIX_EPOCH_S
#define BUILD_UNIX_EPOCH_S 0
#endif

krebb::SkinSensor skinSensor;
krebb::AmbientSensor ambientSensor;
krebb::BleService bleService;
krebb::QualityEstimator quality;
krebb::TimeSource timeSource(static_cast<int64_t>(BUILD_UNIX_EPOCH_S) * 1000LL);

// Fixed buffers: nothing in the 1 Hz path touches the heap.
char packetBuffer[PACKET_BUFFER_SIZE];
char statusLine[256];
char serialCommand[32];
size_t serialCommandLength = 0;

uint32_t nextTickAtMs = 0;
bool conversionStartedThisTick = false;

void printBanner() {
  Serial.println();
  Serial.println("=====================================================================");
  Serial.printf("Krebb One firmware v%s\n", FIRMWARE_VERSION);
  Serial.printf("build epoch: %lu (dev-only clock fallback)\n",
                static_cast<unsigned long>(BUILD_UNIX_EPOCH_S));
  Serial.println("---------------------------------------------------------------------");
  Serial.printf("pin map: DS18B20 DQ  = GPIO%d  (1-Wire, external 4.7k pull-up, powered)\n",
                krebb::PIN_SKIN_ONEWIRE);
  Serial.printf("         DHT11 DATA  = GPIO%d  (3-pin module, on-board pull-up)\n",
                krebb::PIN_AMBIENT_DHT11);
  Serial.println("         both sensors powered from 3V3, common GND");
  Serial.println("---------------------------------------------------------------------");
  Serial.println("BLE service        F000A001-0451-4000-B000-000000000000");
  Serial.println("    measurement    F000A002-0451-4000-B000-000000000000  (notify)");
  Serial.println("    device name    Krebb One");
  Serial.println("---------------------------------------------------------------------");
  Serial.println("Krebb One is a research prototype, NOT a medical device.");
  Serial.println("The DS18B20 reports a skin-contact temperature trend only: it does");
  Serial.println("not measure calories, metabolic rate, or clinical body temperature.");
  Serial.println("---------------------------------------------------------------------");
  Serial.println("serial commands:  t <unix_ms>   set the clock (dev convenience)");
  Serial.println("=====================================================================");
  Serial.println();
}

// The only serial command. Deliberately not a BLE characteristic: adding one
// would be a change to the shared contract. See README, "Time and timestampMs".
void handleSerialCommand(const char* command, uint32_t nowMs) {
  if (command[0] != 't' || (command[1] != ' ' && command[1] != '\0')) {
    Serial.printf("unknown command '%s' (only 't <unix_ms>' is supported)\n", command);
    return;
  }

  const long long value = atoll(command + 1);
  if (value <= 0) {
    Serial.println("usage: t <unix_ms>   e.g. t 1760000000000");
    return;
  }

  timeSource.setEpochMs(static_cast<int64_t>(value), nowMs);
  Serial.printf("clock set: TIME=SYNCED, now = %lld ms\n",
                static_cast<long long>(timeSource.nowUnixMs(nowMs)));
}

void pumpSerial(uint32_t nowMs) {
  while (Serial.available() > 0) {
    const int c = Serial.read();
    if (c < 0) {
      break;
    }
    if (c == '\r') {
      continue;
    }
    if (c == '\n') {
      serialCommand[serialCommandLength] = '\0';
      if (serialCommandLength > 0) {
        handleSerialCommand(serialCommand, nowMs);
      }
      serialCommandLength = 0;
      continue;
    }
    if (serialCommandLength + 1 < sizeof(serialCommand)) {
      serialCommand[serialCommandLength++] = static_cast<char>(c);
    }
    // Overlong input is dropped rather than overflowing the buffer.
  }
}

void logSensorErrors(const krebb::AmbientSelection& ambient) {
  if (!skinSensor.valid()) {
    Serial.printf("  skin: %s\n", krebb::skinStatusMessage(skinSensor.status()));
  }
  if (ambientSensor.lastAttemptFailed()) {
    Serial.printf("  amb: %s\n", ambientSensor.lastError());
  }
  if (!ambient.present && ambientSensor.lastValidSample().valid) {
    Serial.printf("  amb: last valid reading is %.1fs old (max %.1fs) - sending null\n",
                  ambient.ageMs / 1000.0f, AMBIENT_MAX_AGE_MS / 1000.0f);
  }
}

void printStatusLine(uint32_t nowMs, const krebb::AmbientSelection& ambient) {
  char skinField[32];
  if (skinSensor.valid()) {
    snprintf(skinField, sizeof(skinField), "skin=%.2fC ok", skinSensor.celsius());
  } else {
    snprintf(skinField, sizeof(skinField), "skin=null %s",
             skinSensor.devicePresent() ? "ERR" : "NO_DEVICE");
  }

  char ambientField[48];
  if (ambient.present) {
    snprintf(ambientField, sizeof(ambientField), "amb=%.1fC age=%.1fs",
             ambient.celsius, ambient.ageMs / 1000.0f);
  } else {
    snprintf(ambientField, sizeof(ambientField), "amb=null age=%.1fs",
             ambient.ageMs / 1000.0f);
  }

  char humidityField[24];
  if (ambientSensor.hasHumidity()) {
    // Serial only. Humidity is not part of the BLE contract.
    snprintf(humidityField, sizeof(humidityField), " hum=%.0f%%",
             ambientSensor.humidityPercent());
  } else {
    humidityField[0] = '\0';
  }

  snprintf(statusLine, sizeof(statusLine),
           "[%05lus] %s | %s%s | q=%.2f %s win_range=%.2fC consec=%lus | "
           "ble=%s mtu=%u | TIME=%s",
           static_cast<unsigned long>(nowMs / 1000UL), skinField, ambientField,
           humidityField, quality.value(),
           krebb::qualityReasonName(quality.reason()), quality.windowRangeC(),
           static_cast<unsigned long>(quality.consecutiveValidS()),
           krebb::bleStateName(bleService.state()),
           static_cast<unsigned>(bleService.mtu()),
           timeSource.isSynced() ? "SYNCED" : "UNSYNCED(dev-fallback)");

  Serial.println(statusLine);
}

// One 1 Hz tick: collect, score, serialise, notify, log.
void onTick(uint32_t nowMs) {
  // 1. Collect the conversion started ~850 ms ago.
  skinSensor.readLatest(nowMs);

  // 2. Score contact quality from the skin channel alone.
  quality.update(skinSensor.valid(), skinSensor.celsius());

  // 3. Decide whether the last ambient reading is still fresh enough to send.
  const krebb::AmbientSelection ambient = krebb::selectAmbientForPacket(
      ambientSensor.lastValidSample(), nowMs, AMBIENT_MAX_AGE_MS);

  // 4. Assemble the packet. An invalid reading is null, never a stale value.
  krebb::PacketInput input;
  input.timestampMs = timeSource.nowUnixMs(nowMs);
  input.skinValid = skinSensor.valid();
  input.skinTemperatureC = skinSensor.celsius();
  input.ambientValid = ambient.present;
  input.ambientTemperatureC = ambient.celsius;
  input.sensorQuality = quality.value();

  const size_t length =
      krebb::formatMeasurementPacket(packetBuffer, sizeof(packetBuffer), input);

  if (length == 0) {
    Serial.println("PACKET ERROR: could not serialise the measurement packet");
    return;
  }

  // 5. Notify only while a client is subscribed.
  bleService.notify(packetBuffer, length);

  // 6. Log. The PKT line is the exact bytes that were assembled, and is
  //    byte-identical to the notification whenever ble=SUBSCRIBED, so real
  //    example packets can be copied straight out of the monitor.
  logSensorErrors(ambient);
  Serial.printf("PKT %s\n", packetBuffer);
  printStatusLine(nowMs, ambient);
}

}  // namespace

void setup() {
  Serial.begin(115200);
  // Brief, one-off: gives the USB-to-UART bridge time to come up so the banner
  // is not lost. Never done inside loop().
  delay(300);

  const uint32_t now = millis();
  printBanner();

  skinSensor.begin(now);
  ambientSensor.begin(now);
  bleService.begin();

  Serial.println("advertising as \"Krebb One\" - waiting for a client to subscribe");
  Serial.println();

  nextTickAtMs = now + PACKET_PERIOD_MS;
}

void loop() {
  const uint32_t now = millis();

  pumpSerial(now);
  bleService.loop();
  // Re-scans the 1-Wire bus while no DS18B20 is present, so a sensor knocked
  // off the breadboard recovers without a reboot.
  skinSensor.service(now);

  // Fixed-cadence scheduler: the next tick is scheduled from the previous
  // deadline rather than from "now", so the 1 Hz notify does not drift.
  // Signed comparison is rollover-safe.
  if (static_cast<int32_t>(now - nextTickAtMs) >= 0) {
    nextTickAtMs += PACKET_PERIOD_MS;
    // If something blocked for longer than a whole period, skip the backlog
    // instead of firing a burst of catch-up packets at the app.
    if (static_cast<int32_t>(now - nextTickAtMs) >= 0) {
      nextTickAtMs = now + PACKET_PERIOD_MS;
    }
    conversionStartedThisTick = false;
    onTick(now);
  }

  // Milliseconds since the last tick fired.
  const uint32_t msSinceTick = now - (nextTickAtMs - PACKET_PERIOD_MS);

  if (!conversionStartedThisTick && msSinceTick >= CONVERSION_START_OFFSET_MS) {
    skinSensor.startConversion(now);
    conversionStartedThisTick = true;
  }

  ambientSensor.service(now, msSinceTick);

  // Keeps the loop from spinning flat out without delaying the scheduler.
  delay(1);
}
