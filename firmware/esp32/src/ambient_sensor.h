#pragma once

#include <stdint.h>

#include "freshness.h"

// DHT11 ambient (room) temperature for Krebb One.
//
// Ambient temperature is context, not a correction factor. It exists so the
// app/ML side can spot confounders - an HVAC cycle, the device being handled,
// skin-contact loss, the sensor ending up in room air. No physics correction
// is computed from it here or anywhere else in the firmware.
//
// The part is coarse: 1 degC resolution, about +/-2 degC accuracy, 0-50 degC
// range (Aosong datasheet). Treat only multi-degree trends as real.
//
// The module in use is sold as "CNT5" but is a DHT11 on a 3-pin carrier with
// its own pull-up, so there is no external resistor on this net.
//
// It also reports humidity. Humidity is NOT part of the BLE contract and is
// never transmitted; it is logged to serial only, as a debugging aid.

namespace krebb {

class AmbientSensor {
 public:
  void begin(uint32_t nowMs);

  // Attempts a read when both the minimum sampling interval has elapsed and
  // the phase window inside the 1 Hz tick is clear. Returns true if a read was
  // attempted this call (successful or not).
  bool service(uint32_t nowMs, uint32_t msSinceTick);

  // Most recent VALID reading, for the freshness rule. Never overwritten by a
  // failed read, and never sent unless selectAmbientForPacket() allows it.
  const AmbientSample& lastValidSample() const { return lastValid_; }

  // Result of the most recent attempt, for the serial log.
  bool lastAttemptFailed() const { return lastAttemptFailed_; }
  const char* lastError() const { return lastError_; }

  // Serial-only debug value. Never goes into the packet.
  bool hasHumidity() const { return hasHumidity_; }
  float humidityPercent() const { return humidityPercent_; }

 private:
  AmbientSample lastValid_;
  uint32_t lastAttemptAtMs_ = 0;
  bool firstReadDue_ = false;
  uint32_t readyAtMs_ = 0;
  bool lastAttemptFailed_ = false;
  const char* lastError_ = "";
  bool hasHumidity_ = false;
  float humidityPercent_ = 0.0f;
};

}  // namespace krebb
