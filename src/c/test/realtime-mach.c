//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <mach/clock.h>
#include <mach/mach.h>

// Returns the current time since epoch counted in seconds.
double get_current_time_seconds() {
  clock_serv_t clock_serv;
  mach_timespec_t spec;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &clock_serv);
  clock_get_time(clock_serv, &spec);
  mach_port_deallocate(mach_task_self(), clock_serv);
  return spec.tv_sec + (spec.tv_nsec / 1000000000.0);
}
