#include "status_line.h"

#include <math.h>
#include <stdio.h>

namespace krebb {
namespace {

// Per-group scratch. The widest group is the quality one, and these are sized
// with room to spare: an over-long group is truncated inside its own buffer
// and can never eat the separator that follows it.
constexpr size_t GROUP_BUFFER_SIZE = 96;

constexpr const char* SEPARATOR = " | ";
constexpr const char* NO_VALUE = "--";

// Appends one group, preceded by the separator unless it is the first.
//
// This is the whole point of the module: the separator is written here and
// nowhere else, so no combination of field widths or state names can produce
// "ble=ADVERTISINGmtu=0" or "mtu=0 |TIME=...". If a group would not fit, the
// append fails and the caller reports the line as unbuildable rather than
// handing back a half-joined one.
bool appendGroup(char* out, size_t outSize, size_t& length, uint8_t& groupCount,
                 const char* group) {
  const char* prefix = (groupCount == 0) ? "" : SEPARATOR;
  const int written =
      snprintf(out + length, outSize - length, "%s%s", prefix, group);
  if (written <= 0 || static_cast<size_t>(written) >= outSize - length) {
    return false;
  }
  length += static_cast<size_t>(written);
  ++groupCount;
  return true;
}

// snprintf's %f honours the C locale's decimal separator, so a comma locale
// would render "33,42" here. That is cosmetic in a serial log - unlike the BLE
// packet, which is JSON and renders its numbers from scaled integers for
// exactly this reason (see packet.cpp) - but the host tests assert exact bytes
// under the default C locale, and the firmware never calls setlocale().
void writeSkin(char* out, size_t outSize, const StatusLineInput& in) {
  if (in.skinValid && isfinite(in.skinCelsius)) {
    snprintf(out, outSize, "skin=%.2fC ok", static_cast<double>(in.skinCelsius));
  } else {
    snprintf(out, outSize, "skin=null %s",
             in.skinDevicePresent ? "ERR" : "NO_DEVICE");
  }
}

void writeAmbient(char* out, size_t outSize, const StatusLineInput& in) {
  char value[24];
  if (in.ambientPresent && isfinite(in.ambientCelsius)) {
    snprintf(value, sizeof(value), "amb=%.1fC",
             static_cast<double>(in.ambientCelsius));
  } else {
    snprintf(value, sizeof(value), "amb=null");
  }

  // Humidity rides on the same DHT11 frame as the ambient temperature. With no
  // fresh frame there is no fresh humidity either, and printing the last one -
  // or, as the firmware used to, a rejected sensor's 0 - reads as a live
  // measurement. "--" says "not available" and cannot be mistaken for one.
  char humidity[24];
  if (in.ambientPresent && in.humidityPresent && isfinite(in.humidityPercent)) {
    snprintf(humidity, sizeof(humidity), "hum=%.0f%%",
             static_cast<double>(in.humidityPercent));
  } else {
    snprintf(humidity, sizeof(humidity), "hum=%s", NO_VALUE);
  }

  snprintf(out, outSize, "%s age=%.1fs %s", value, in.ambientAgeMs / 1000.0f,
           humidity);
}

void writeQuality(char* out, size_t outSize, const StatusLineInput& in) {
  snprintf(out, outSize, "q=%.2f %s win_range=%.2fC consec=%lus",
           static_cast<double>(in.quality),
           in.qualityReason == nullptr ? "" : in.qualityReason,
           static_cast<double>(in.windowRangeC),
           static_cast<unsigned long>(in.consecutiveValidS));
}

}  // namespace

size_t formatStatusLine(char* out, size_t outSize, const StatusLineInput& in) {
  if (out == nullptr || outSize == 0) {
    return 0;
  }
  out[0] = '\0';

  // The uptime is a prefix rather than a group: it keeps its single trailing
  // space so the line reads the same in the monitor as it always has.
  const int stamped = snprintf(out, outSize, "[%05lus] ",
                               static_cast<unsigned long>(in.uptimeS));
  if (stamped <= 0 || static_cast<size_t>(stamped) >= outSize) {
    out[0] = '\0';
    return 0;
  }

  size_t length = static_cast<size_t>(stamped);
  uint8_t groupCount = 0;
  char group[GROUP_BUFFER_SIZE];

  writeSkin(group, sizeof(group), in);
  if (!appendGroup(out, outSize, length, groupCount, group)) {
    out[0] = '\0';
    return 0;
  }

  writeAmbient(group, sizeof(group), in);
  if (!appendGroup(out, outSize, length, groupCount, group)) {
    out[0] = '\0';
    return 0;
  }

  writeQuality(group, sizeof(group), in);
  if (!appendGroup(out, outSize, length, groupCount, group)) {
    out[0] = '\0';
    return 0;
  }

  snprintf(group, sizeof(group), "ble=%s mtu=%u",
           in.bleState == nullptr ? "" : in.bleState,
           static_cast<unsigned>(in.mtu));
  if (!appendGroup(out, outSize, length, groupCount, group)) {
    out[0] = '\0';
    return 0;
  }

  snprintf(group, sizeof(group), "TIME=%s",
           in.timeSynced ? "SYNCED" : "UNSYNCED(dev-fallback)");
  if (!appendGroup(out, outSize, length, groupCount, group)) {
    out[0] = '\0';
    return 0;
  }

  return length;
}

}  // namespace krebb
