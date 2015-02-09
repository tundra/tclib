//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#define __USE_POSIX199309
#include <time.h>

// Returns the current time since epoch counted in seconds.
double get_current_time_seconds() {
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  return spec.tv_sec + (spec.tv_nsec / 1000000000.0);
}
