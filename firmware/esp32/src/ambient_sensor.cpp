#include "ambient_sensor.h"

#include <Arduino.h>
#include <DHTesp.h>
#include <math.h>

#include "krebb_params.h"
#include "pins.h"

namespace krebb {
namespace {

using namespace krebb::params;

DHTesp dht;

}  // namespace

void AmbientSensor::begin(uint32_t nowMs) {
  dht.setup(PIN_AMBIENT_DHT11, DHTesp::DHT11);

  // Datasheet: the part needs ~1 s after power-up before it will answer, and
  // reading it earlier just produces a spurious first failure in the log.
  readyAtMs_ = nowMs + AMBIENT_FIRST_READ_DELAY_MS;
  firstReadDue_ = true;
  lastAttemptAtMs_ = nowMs;
}

bool AmbientSensor::service(uint32_t nowMs, uint32_t msSinceTick) {
  if (firstReadDue_) {
    if (static_cast<int32_t>(nowMs - readyAtMs_) < 0) {
      return false;
    }
  } else if ((nowMs - lastAttemptAtMs_) < AMBIENT_SAMPLE_INTERVAL_MS) {
    // Datasheet minimum sampling period is 1 s; 2 s is used for margin, and
    // the ambient signal is slow-moving context anyway.
    return false;
  }

  // A DHT11 read bit-bangs the line for ~25 ms with interrupts disabled. Doing
  // that during packet assembly would add jitter to the 1 Hz notify, and doing
  // it while the DS18B20 conversion is being kicked off risks disturbing that
  // bus, so reads are confined to a quiet phase window inside the tick.
  if (msSinceTick < AMBIENT_PHASE_MIN_MS || msSinceTick > AMBIENT_PHASE_MAX_MS) {
    return false;
  }

  firstReadDue_ = false;
  lastAttemptAtMs_ = nowMs;

  const TempAndHumidity reading = dht.getTempAndHumidity();
  const DHTesp::DHT_ERROR_t error = dht.getStatus();

  if (error != DHTesp::ERROR_NONE) {
    lastAttemptFailed_ = true;
    lastError_ = (error == DHTesp::ERROR_CHECKSUM) ? "DHT11 checksum failed"
                                                   : "DHT11 timeout / no response";
    return true;
  }

  if (isnan(reading.temperature)) {
    lastAttemptFailed_ = true;
    lastError_ = "DHT11 returned NaN";
    return true;
  }

  if (reading.temperature < AMBIENT_SANITY_MIN_C ||
      reading.temperature > AMBIENT_SANITY_MAX_C) {
    lastAttemptFailed_ = true;
    lastError_ = "DHT11 reading outside the 0-50 degC operating range";
    return true;
  }

  lastAttemptFailed_ = false;
  lastError_ = "";

  // Only a valid reading is stored; a failure leaves the previous one in place
  // and lets the freshness rule decide whether it may still be sent.
  lastValid_.valid = true;
  lastValid_.celsius = reading.temperature;
  lastValid_.observedAtMs = nowMs;

  // Humidity is captured for the serial log only. It is deliberately not part
  // of the BLE packet and must never be added to it.
  if (!isnan(reading.humidity)) {
    hasHumidity_ = true;
    humidityPercent_ = reading.humidity;
  }

  return true;
}

}  // namespace krebb
