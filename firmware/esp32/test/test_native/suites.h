#pragma once

// Each translation unit contributes one suite; main.cpp runs them all under a
// single Unity session so `pio test -e native` reports one result.

void run_packet_tests();
void run_status_line_tests();
void run_ambient_validate_tests();
void run_quality_tests();
void run_freshness_tests();
void run_time_source_tests();
void run_scenario_tests();
