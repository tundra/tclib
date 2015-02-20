//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "realtime.h"
#include "utils/clock.h"

double get_current_time_seconds() {
  real_time_clock_t *system_time = real_time_clock_system();
  return real_time_clock_millis_since_epoch_utc(system_time) / 1000.0;
}
