#pragma once

#include <stdint.h>

// DS18B20 skin-contact temperature acquisition for Krebb One.
//
// This reports a local skin-contact temperature trend. It is NOT a clinical
// body temperature, and it says nothing about calories or metabolic rate.
// Only the within-session change from baseline is meaningful.
//
// Powered mode (VDD to 3V3), 12-bit, asynchronous conversion. The sensor may
// be on flexible leads against skin, so an intermittent or absent device is a
// normal operating condition, not an error to crash on.

namespace krebb {

enum class SkinStatus : uint8_t {
  OK,
  NO_DEVICE,           // nothing answered the 1-Wire search
  CONVERSION_PENDING,  // conversion had not finished by the read deadline
  DISCONNECTED_READ,   // driver returned DEVICE_DISCONNECTED_C (-127)
  POWER_ON_RESET,      // scratchpad still holds the 85.0 power-on default
  NOT_A_NUMBER,        // NaN from the driver
  OUT_OF_RANGE,        // outside the sensor sanity window
};

// Human-readable explanation for the serial log.
const char* skinStatusMessage(SkinStatus status);

class SkinSensor {
 public:
  // Sets up the bus and looks for a device. Safe to call once from setup().
  void begin(uint32_t nowMs);

  // Re-scans the bus roughly once a second while no device is present, so a
  // sensor that was knocked loose can be re-seated without a reboot.
  void service(uint32_t nowMs);

  // Kicks off a conversion. Called CONVERSION_START_OFFSET_MS after each tick.
  void startConversion(uint32_t nowMs);

  // Collects the result of the conversion started in the previous second.
  // Called at the tick, immediately before the packet is assembled.
  void readLatest(uint32_t nowMs);

  bool valid() const { return status_ == SkinStatus::OK; }
  float celsius() const { return celsius_; }
  SkinStatus status() const { return status_; }
  bool devicePresent() const { return devicePresent_; }

  // millis() at which the latest conversion completed, for the age comment in
  // the serial log.
  uint32_t lastReadAtMs() const { return lastReadAtMs_; }

 private:
  bool initialiseBus(uint32_t nowMs);

  float celsius_ = 0.0f;
  SkinStatus status_ = SkinStatus::NO_DEVICE;
  bool devicePresent_ = false;
  bool conversionInFlight_ = false;
  uint32_t conversionStartedAtMs_ = 0;
  uint32_t lastReadAtMs_ = 0;
  uint32_t lastInitAttemptMs_ = 0;
};

}  // namespace krebb
