//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "realtime.h"
#include "utils/clock.h"

double get_current_time_seconds() {
  real_time_clock_t *system_time = real_time_clock_system();
  native_time_t time = real_time_clock_time_since_epoch_utc(system_time);
  uint64_t millis = native_time_to_millis(time);
  return ((double) millis) / 1000.0;
}
