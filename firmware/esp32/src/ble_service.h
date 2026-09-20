#pragma once

#include <stddef.h>
#include <stdint.h>

// Krebb One BLE peripheral.
//
// Advertises one custom service with a single NOTIFY characteristic carrying
// the UTF-8 JSON measurement packet. The UUIDs and the packet shape are the
// shared contract with the iOS app - see docs/sensor-protocol.md and
// docs/BLE_PROTOCOL.md. They are not to be changed on this side alone.
//
// An "active session" is a connected client that has subscribed to
// notifications. While subscribed the firmware notifies once per second; when
// nothing is subscribed it sends nothing at all. Sensors keep sampling either
// way, so the quality window is already warm when a session starts.

namespace krebb {

enum class BleState : uint8_t {
  ADVERTISING,
  CONNECTED,   // connected but not subscribed: no packets are sent
  SUBSCRIBED,  // active session
};

const char* bleStateName(BleState state);

class BleService {
 public:
  void begin();

  // Drains callback events into the serial log and keeps advertising alive.
  // Called every loop iteration from the main loop, never from a callback.
  void loop();

  BleState state() const;
  bool isSubscribed() const;

  // Negotiated ATT MTU, or 0 when not connected.
  uint16_t mtu() const;

  // Sends one notification. Returns false (and logs loudly) if the payload
  // would not fit the negotiated MTU: a truncated packet is worse than a
  // missing one, because the app would parse half a JSON object.
  bool notify(const char* payload, size_t length);
};

}  // namespace krebb
