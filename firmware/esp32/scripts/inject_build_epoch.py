"""Inject the build time as BUILD_UNIX_EPOCH_S.

Krebb One has no RTC and no NTP, but the BLE contract requires timestampMs to
be Unix milliseconds and never null. Until the clock is set at runtime (serial
`t <unix_ms>`), the firmware falls back to BUILD_UNIX_EPOCH_S * 1000 + millis().

That fallback is DEV-ONLY: it is only as correct as "the moment this firmware
was compiled", so the firmware reports TIME=UNSYNCED(dev-fallback) until a real
epoch is supplied. See README.md, "Time and timestampMs".

PlatformIO runs this as a `pre:` extra_script for env:esp32s3.
"""

import time

Import("env")  # noqa: F821  - injected by PlatformIO's SCons environment

build_epoch_s = int(time.time())

env.Append(CPPDEFINES=[("BUILD_UNIX_EPOCH_S", build_epoch_s)])  # noqa: F821

print(
    "krebb: BUILD_UNIX_EPOCH_S=%d (%s UTC) -- dev-only clock fallback"
    % (build_epoch_s, time.strftime("%Y-%m-%d %H:%M:%S", time.gmtime(build_epoch_s)))
)
