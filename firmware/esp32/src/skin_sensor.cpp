#include "skin_sensor.h"

#include <Arduino.h>
#include <DallasTemperature.h>
#include <OneWire.h>
#include <math.h>

#include "krebb_params.h"
#include "pins.h"

namespace krebb {
namespace {

using namespace krebb::params;

OneWire oneWire(PIN_SKIN_ONEWIRE);
DallasTemperature dallas(&oneWire);
DeviceAddress deviceAddress;

}  // namespace

const char* skinStatusMessage(SkinStatus status) {
  switch (status) {
    case SkinStatus::OK:
      return "ok";
    case SkinStatus::NO_DEVICE:
      return "no DS18B20 found on the 1-Wire bus (check DQ, 4.7k pull-up, GND)";
    case SkinStatus::CONVERSION_PENDING:
      return "conversion not complete at the read deadline";
    case SkinStatus::DISCONNECTED_READ:
      return "DS18B20 disconnected (-127)";
    case SkinStatus::POWER_ON_RESET:
      return "power-on-reset value 85.0 rejected";
    case SkinStatus::NOT_A_NUMBER:
      return "driver returned NaN";
    case SkinStatus::OUT_OF_RANGE:
      return "reading outside the sensor sanity window";
  }
  return "unknown";
}

bool SkinSensor::initialiseBus(uint32_t nowMs) {
  lastInitAttemptMs_ = nowMs;

  dallas.begin();
  // Powered mode: the bus is free during conversion, so nothing has to hold a
  // strong pull-up and the loop can keep running while the part converts.
  dallas.setWaitForConversion(false);

  if (dallas.getDeviceCount() < 1 || !dallas.getAddress(deviceAddress, 0)) {
    devicePresent_ = false;
    conversionInFlight_ = false;
    return false;
  }

  dallas.setResolution(deviceAddress, SKIN_RESOLUTION_BITS);
  devicePresent_ = true;
  return true;
}

void SkinSensor::begin(uint32_t nowMs) {
  status_ = SkinStatus::NO_DEVICE;
  initialiseBus(nowMs);
}

void SkinSensor::service(uint32_t nowMs) {
  if (devicePresent_) {
    return;
  }
  // Unsigned subtraction: correct across the millis() rollover.
  if ((nowMs - lastInitAttemptMs_) < SKIN_REINIT_INTERVAL_MS) {
    return;
  }
  initialiseBus(nowMs);
}

void SkinSensor::startConversion(uint32_t nowMs) {
  if (!devicePresent_) {
    return;
  }
  // Timing budget, per tick (see krebb_params.h):
  //   t+0    ms  tick: read the conversion started last second
  //   t+150  ms  start this conversion   (CONVERSION_START_OFFSET_MS)
  //   t+900  ms  worst-case completion   (+750 ms, 12-bit max per datasheet)
  //   t+1000 ms  next tick reads it      -> 100 ms of margin
  // So the value in each packet was measured within ~100 ms of being sent,
  // and never more than ~850 ms old even if the part finishes early.
  //
  // If the bench shows CONVERSION_PENDING errors, the fix is to reduce
  // CONVERSION_START_OFFSET_MS, or drop to 11-bit (375 ms, 0.125 degC steps)
  // which would widen the margin to 475 ms.
  dallas.requestTemperaturesByAddress(deviceAddress);
  conversionInFlight_ = true;
  conversionStartedAtMs_ = nowMs;
}

void SkinSensor::readLatest(uint32_t nowMs) {
  if (!devicePresent_) {
    status_ = SkinStatus::NO_DEVICE;
    return;
  }

  if (!conversionInFlight_) {
    // First tick after boot or after a re-init: nothing was requested yet.
    status_ = SkinStatus::CONVERSION_PENDING;
    return;
  }

  if (!dallas.isConversionComplete()) {
    status_ = SkinStatus::CONVERSION_PENDING;
    return;
  }

  conversionInFlight_ = false;
  const float reading = dallas.getTempC(deviceAddress);
  lastReadAtMs_ = nowMs;

  if (isnan(reading)) {
    status_ = SkinStatus::NOT_A_NUMBER;
    return;
  }

  if (reading <= SKIN_DISCONNECTED_C + 1.0f) {
    // DEVICE_DISCONNECTED_C. Usually a broken lead or a missing pull-up.
    status_ = SkinStatus::DISCONNECTED_READ;
    devicePresent_ = false;  // force a re-scan so a re-seat is picked up
    return;
  }

  if (fabsf(reading - SKIN_POWER_ON_RESET_C) < SKIN_POR_EPSILON_C) {
    // 85.0 exactly is the scratchpad default after a power-on reset: the part
    // browned out or was never converted, so this is not a measurement.
    status_ = SkinStatus::POWER_ON_RESET;
    return;
  }

  if (reading < SKIN_SANITY_MIN_C || reading > SKIN_SANITY_MAX_C) {
    // A wiring/contact fault, not a person. This is a sensor sanity bound,
    // not a physiological claim.
    status_ = SkinStatus::OUT_OF_RANGE;
    return;
  }

  celsius_ = reading;
  status_ = SkinStatus::OK;
}

}  // namespace krebb
