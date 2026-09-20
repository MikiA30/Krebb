#pragma once

#include <stddef.h>
#include <stdint.h>

// All Krebb One tunables in one place, so they can be adjusted from recorded
// bench data without hunting through the firmware.
//
// Krebb One is a research prototype. It is not a medical device, calorimeter,
// or validated calorie tracker. Nothing below is a physiological threshold;
// the temperature limits are sensor sanity bounds only.

namespace krebb {
namespace params {

// ---------------------------------------------------------------- scheduling
// The BLE contract is exactly one packet per second during an active session.
constexpr uint32_t PACKET_PERIOD_MS = 1000;

// The DS18B20 conversion for tick N+1 starts this long after tick N, so the
// value carried by each packet is as freshly measured as the part allows.
// Budget: 150 ms offset + 750 ms max 12-bit conversion = 900 ms, leaving a
// 100 ms margin before the next tick reads it. See skin_sensor.cpp.
constexpr uint32_t CONVERSION_START_OFFSET_MS = 150;

// --------------------------------------------------------- skin sensor (DS18B20)
// 12-bit resolution: 0.0625 degC steps, 750 ms max conversion (datasheet).
constexpr uint8_t SKIN_RESOLUTION_BITS = 12;
constexpr uint32_t SKIN_CONVERSION_MAX_MS = 750;

// Sensor sanity window, NOT a physiological claim. A skin-contact reading
// outside this is a wiring/contact fault, not a person.
constexpr float SKIN_SANITY_MIN_C = 5.0f;
constexpr float SKIN_SANITY_MAX_C = 50.0f;

// DS18B20 sentinel values that must never reach the packet.
constexpr float SKIN_DISCONNECTED_C = -127.0f;  // DEVICE_DISCONNECTED_C
constexpr float SKIN_POWER_ON_RESET_C = 85.0f;  // scratchpad default after POR
constexpr float SKIN_POR_EPSILON_C = 0.01f;     // float compare tolerance

// If no device answers at boot, re-scan the bus at this interval so a sensor
// on flexible skin leads can be re-seated without a reboot.
constexpr uint32_t SKIN_REINIT_INTERVAL_MS = 1000;

// ------------------------------------------------------ ambient sensor (DHT11)
// Datasheet: sampling period must not be shorter than 1 s; 2 s is the safe
// choice, and the ambient signal is slow-moving context anyway.
constexpr uint32_t AMBIENT_SAMPLE_INTERVAL_MS = 2000;

// Datasheet: wait ~1 s after power-up before the first read.
constexpr uint32_t AMBIENT_FIRST_READ_DELAY_MS = 1000;

// A DHT11 read bit-bangs for ~25 ms with interrupts disabled, so it is kept
// away from both the 1 Hz packet assembly and the DS18B20 conversion start by
// only sampling inside this phase window after a tick.
constexpr uint32_t AMBIENT_PHASE_MIN_MS = 300;
constexpr uint32_t AMBIENT_PHASE_MAX_MS = 700;

// The packet carries the most recent VALID ambient reading only while it is
// younger than this; otherwise ambientTemperatureC is null. This is the one
// bounded exception to "one reading per interval" and exists because the DHT11
// samples at 0.5 Hz while the packet goes out at 1 Hz.
constexpr uint32_t AMBIENT_MAX_AGE_MS = 5000;

// DHT11 operating range (datasheet 0-50 degC). Outside this is a failed read.
constexpr float AMBIENT_SANITY_MIN_C = 0.0f;
constexpr float AMBIENT_SANITY_MAX_C = 50.0f;

// DHT11 humidity operating range (datasheet 20-90 %RH). Humidity itself is
// never transmitted, but it is the only cross-check the part offers: a frame
// reporting humidity outside its own specified range did not come from a
// healthy sensor, so the temperature in that same frame is not trusted either.
constexpr float AMBIENT_HUMIDITY_MIN_PCT = 20.0f;
constexpr float AMBIENT_HUMIDITY_MAX_PCT = 90.0f;

// ----------------------------------------------------------------- quality
// sensorQuality is derived ONLY from DS18B20 validity and recent variation.
// Ambient temperature is never an input: it is reported so the app/ML side can
// spot confounders, not folded into a score here.

// Rolling window used for the variation measures, in 1 Hz samples.
constexpr uint8_t QUALITY_WINDOW_S = 20;

// Consecutive valid seconds before contact counts as settled rather than
// warming up after placement.
constexpr uint32_t QUALITY_WARMUP_S = 60;

// Further consecutive valid seconds past warmup for full duration credit.
constexpr uint32_t QUALITY_FULL_S = 120;

// Window variation scoring. At or below "stable" scores 1.0, at or above
// "unstable" scores 0.0, linear in between.
constexpr float QUALITY_RANGE_STABLE_C = 0.10f;
constexpr float QUALITY_RANGE_UNSTABLE_C = 0.60f;
constexpr float QUALITY_STDDEV_STABLE_C = 0.03f;
constexpr float QUALITY_STDDEV_UNSTABLE_C = 0.20f;

// Hysteresis on the UNSTABLE decision. Entry uses the scoring thresholds
// above; leaving UNSTABLE needs the window range to sit at or below this
// tighter bound for this many consecutive 1 Hz samples. Without it, a window
// range hovering either side of QUALITY_RANGE_UNSTABLE_C makes the reported
// state flip every second or two, which the app would see as flicker.
constexpr float QUALITY_UNSTABLE_EXIT_RANGE_C = 0.40f;
constexpr uint32_t QUALITY_UNSTABLE_EXIT_HOLD_S = 5;

// Band edges from the context file's quality heuristic.
constexpr float QUALITY_INVALID = 0.0f;         // disconnected / invalid read
constexpr float QUALITY_STABILIZING_MIN = 0.3f;  // warming or high variation
constexpr float QUALITY_STABILIZING_MAX = 0.6f;
constexpr float QUALITY_STABLE_MIN = 0.7f;       // settled, low variation
constexpr float QUALITY_STABLE_MAX = 1.0f;

// -------------------------------------------------------------------- packet
// Fixed buffer; the packet is ~100-115 bytes and must never be built on the
// heap inside the 1 Hz loop.
constexpr size_t PACKET_BUFFER_SIZE = 128;

// Decimal places on the wire. Fixed so the JSON is byte-stable.
constexpr int PACKET_SKIN_DECIMALS = 2;
constexpr int PACKET_AMBIENT_DECIMALS = 1;
constexpr int PACKET_QUALITY_DECIMALS = 2;

// Defence in depth: a temperature outside the DS18B20's own -55..+125 degC
// range is a bug upstream, and must never be serialised (this is what keeps
// the -127 disconnect sentinel off the wire even if validity flags are wrong).
constexpr float PACKET_HARD_MIN_C = -55.0f;
constexpr float PACKET_HARD_MAX_C = 125.0f;

// ----------------------------------------------------------------------- BLE
constexpr const char* BLE_DEVICE_NAME = "Krebb One";

// The packet is larger than the 20-byte default ATT payload, so a larger MTU
// is requested. Usable notify payload is (negotiated MTU - 3) ATT header bytes.
constexpr uint16_t BLE_PREFERRED_MTU = 247;
constexpr uint16_t BLE_ATT_HEADER_BYTES = 3;

}  // namespace params
}  // namespace krebb
