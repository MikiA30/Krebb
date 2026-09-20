// Host-side unit tests for the Krebb One pure core (lib/krebb_core).
//
// Run with:  pio test -e native
//
// Nothing here touches Arduino or hardware, so these run in well under a
// second and are the fast feedback loop for the packet contract, the
// sensorQuality heuristic, the ambient freshness rule and the clock.

#include <unity.h>

#include "pins.h"
#include "suites.h"

// Tripwire for the pin contract. scripts/check_pins.py is the real check (it
// compares include/pins.h against hardware/wiring/pin-map.md); this catches an
// accidental edit at compile time too.
static_assert(krebb::PIN_SKIN_ONEWIRE == 4, "DS18B20 DQ must stay on GPIO4");
static_assert(krebb::PIN_AMBIENT_DHT11 == 5, "DHT11 DATA must stay on GPIO5");

void setUp() {}
void tearDown() {}

int main(int, char**) {
  UNITY_BEGIN();

  run_packet_tests();
  run_quality_tests();
  run_freshness_tests();
  run_ambient_validate_tests();
  run_status_line_tests();
  run_time_source_tests();
  run_scenario_tests();

  return UNITY_END();
}
