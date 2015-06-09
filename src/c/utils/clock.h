//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_UTILS_CLOCK_H
#define _TCLIB_UTILS_CLOCK_H

#include "c/stdc.h"

#include "sync/sync.h"

// Opaque type representing a real time clock.
typedef struct real_time_clock_t real_time_clock_t;

typedef struct {
  platform_time_t time;
} native_time_t;

// Returns the system real time clock.
real_time_clock_t *real_time_clock_system();

// Returns the number of milliseconds since unix epoch.
native_time_t real_time_clock_time_since_epoch_utc(real_time_clock_t *clock);

// Returns the number of milliseconds represented by the given native time
// object.
uint64_t native_time_to_millis(native_time_t time);

#endif // _TCLIB_UTILS_CLOCK_H
