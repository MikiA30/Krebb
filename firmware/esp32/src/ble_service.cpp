#include "ble_service.h"

#include <Arduino.h>
#include <NimBLEDevice.h>

#include <atomic>

#include "krebb_params.h"

namespace krebb {
namespace {

using namespace krebb::params;

// ---------------------------------------------------------------------------
// Shared contract with the iOS app. Defined once, here.
// Source of truth: docs/sensor-protocol.md and docs/BLE_PROTOCOL.md.
// Changing either value breaks the app and must be agreed with the software
// lead and written into those documents first.
// ---------------------------------------------------------------------------
constexpr const char* SERVICE_UUID = "F000A001-0451-4000-B000-000000000000";
constexpr const char* MEASUREMENT_CHAR_UUID = "F000A002-0451-4000-B000-000000000000";

NimBLEServer* server = nullptr;
NimBLECharacteristic* measurementChar = nullptr;

// Callbacks run on the NimBLE host task, so anything they touch that the main
// loop also reads is atomic, and they only ever set state: all logging and all
// notifying happens on the main loop.
std::atomic<bool> gConnected{false};
std::atomic<bool> gSubscribed{false};
std::atomic<uint16_t> gMtu{0};
std::atomic<uint16_t> gConnHandle{0};

// Pending events for the loop to log.
std::atomic<bool> gEvtConnect{false};
std::atomic<bool> gEvtDisconnect{false};
std::atomic<bool> gEvtSubscribe{false};
std::atomic<bool> gEvtUnsubscribe{false};
std::atomic<bool> gEvtMtu{false};

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* /*pServer*/, ble_gap_conn_desc* desc) override {
    gConnHandle.store(desc->conn_handle);
    gConnected.store(true);
    gEvtConnect.store(true);
  }

  void onDisconnect(NimBLEServer* /*pServer*/) override {
    gConnected.store(false);
    gSubscribed.store(false);
    gMtu.store(0);
    gEvtDisconnect.store(true);
  }

  void onMTUChange(uint16_t MTU, ble_gap_conn_desc* /*desc*/) override {
    gMtu.store(MTU);
    gEvtMtu.store(true);
  }
};

class MeasurementCallbacks : public NimBLECharacteristicCallbacks {
  void onSubscribe(NimBLECharacteristic* /*pCharacteristic*/,
                   ble_gap_conn_desc* /*desc*/, uint16_t subValue) override {
    // Bit 0 is notifications, bit 1 is indications. The contract uses notify.
    const bool subscribed = (subValue & 0x0001) != 0;
    gSubscribed.store(subscribed);
    if (subscribed) {
      gEvtSubscribe.store(true);
    } else {
      gEvtUnsubscribe.store(true);
    }
  }
};

ServerCallbacks serverCallbacks;
MeasurementCallbacks measurementCallbacks;

// Drains an event flag, returning true at most once per occurrence.
bool take(std::atomic<bool>& flag) { return flag.exchange(false); }

}  // namespace

const char* bleStateName(BleState state) {
  switch (state) {
    case BleState::ADVERTISING: return "ADVERTISING";
    case BleState::CONNECTED:   return "CONNECTED";
    case BleState::SUBSCRIBED:  return "SUBSCRIBED";
  }
  return "ADVERTISING";
}

void BleService::begin() {
  NimBLEDevice::init(BLE_DEVICE_NAME);

  // The packet is ~100-115 bytes, well over the 20-byte payload a default
  // 23-byte MTU allows, so a larger MTU is requested up front. The central
  // decides the final value; notify() re-checks it before every send.
  NimBLEDevice::setMTU(BLE_PREFERRED_MTU);

  server = NimBLEDevice::createServer();
  server->setCallbacks(&serverCallbacks);
  // Get back on the air by itself after the phone walks away.
  server->advertiseOnDisconnect(true);

  NimBLEService* service = server->createService(SERVICE_UUID);
  measurementChar =
      service->createCharacteristic(MEASUREMENT_CHAR_UUID, NIMBLE_PROPERTY::NOTIFY);
  measurementChar->setCallbacks(&measurementCallbacks);
  service->start();

  // Service UUID in the advertisement, name in the scan response: a 128-bit
  // UUID is 18 of the 31 advertising bytes, so the name would not reliably fit
  // alongside it.
  NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();

  NimBLEAdvertisementData advertisementData;
  advertisementData.setFlags(BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP);
  advertisementData.setCompleteServices(NimBLEUUID(SERVICE_UUID));
  advertising->setAdvertisementData(advertisementData);

  NimBLEAdvertisementData scanResponseData;
  scanResponseData.setName(BLE_DEVICE_NAME);
  advertising->setScanResponseData(scanResponseData);
  advertising->setScanResponse(true);

  advertising->start();
}

void BleService::loop() {
  if (take(gEvtConnect)) {
    Serial.printf("BLE: connected (handle %u), waiting for notify subscription\n",
                  static_cast<unsigned>(gConnHandle.load()));
  }
  if (take(gEvtMtu)) {
    Serial.printf("BLE: MTU negotiated = %u (usable notify payload %u bytes)\n",
                  static_cast<unsigned>(gMtu.load()),
                  static_cast<unsigned>(gMtu.load() > BLE_ATT_HEADER_BYTES
                                            ? gMtu.load() - BLE_ATT_HEADER_BYTES
                                            : 0));
  }
  if (take(gEvtSubscribe)) {
    Serial.println("BLE: client subscribed - session active, notifying at 1 Hz");
  }
  if (take(gEvtUnsubscribe)) {
    Serial.println("BLE: client unsubscribed - session inactive, sending nothing");
  }
  if (take(gEvtDisconnect)) {
    Serial.println("BLE: disconnected - re-advertising");
  }

  // advertiseOnDisconnect() normally handles this; this is the belt-and-braces
  // path so a missed restart cannot leave the device invisible for a whole
  // demo. Starting an already-started advertisement is a no-op.
  if (!gConnected.load()) {
    NimBLEAdvertising* advertising = NimBLEDevice::getAdvertising();
    if (advertising != nullptr && !advertising->isAdvertising()) {
      advertising->start();
    }
  }
}

BleState BleService::state() const {
  if (gSubscribed.load()) {
    return BleState::SUBSCRIBED;
  }
  return gConnected.load() ? BleState::CONNECTED : BleState::ADVERTISING;
}

bool BleService::isSubscribed() const { return gSubscribed.load(); }

uint16_t BleService::mtu() const {
  const uint16_t cached = gMtu.load();
  if (cached != 0) {
    return cached;
  }
  if (gConnected.load() && server != nullptr) {
    return server->getPeerMTU(gConnHandle.load());
  }
  return 0;
}

bool BleService::notify(const char* payload, size_t length) {
  if (measurementChar == nullptr || payload == nullptr || length == 0) {
    return false;
  }
  if (!gSubscribed.load()) {
    return false;  // no active session: send nothing
  }

  const uint16_t negotiated = mtu();
  const uint16_t usable =
      (negotiated > BLE_ATT_HEADER_BYTES) ? (negotiated - BLE_ATT_HEADER_BYTES) : 0;

  if (length > usable) {
    // Loud on purpose: a silently truncated packet would reach the app as
    // unparseable JSON and look like a firmware bug on their side.
    Serial.printf(
        "BLE ERROR: packet is %u bytes but only %u fit the negotiated MTU (%u). "
        "NOT sending a truncated packet.\n",
        static_cast<unsigned>(length), static_cast<unsigned>(usable),
        static_cast<unsigned>(negotiated));
    return false;
  }

  measurementChar->setValue(reinterpret_cast<const uint8_t*>(payload), length);
  measurementChar->notify();
  return true;
}

}  // namespace krebb
