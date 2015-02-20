//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <time.h>

uint64_t SystemRealTimeClock::millis_since_epoch_utc() {
  struct timespec spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  return static_cast<uint64_t>((spec.tv_sec * 1000.0) + (spec.tv_nsec / 1000000.0));
}
