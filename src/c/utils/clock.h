//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_UTILS_CLOCK_H
#define _TCLIB_UTILS_CLOCK_H

#include "c/stdc.h"

// Opaque type representing a real time clock.
typedef struct real_time_clock_t real_time_clock_t;

// Returns the system real time clock.
real_time_clock_t *real_time_clock_system();

// Returns the number of milliseconds since unix epoch.
uint64_t real_time_clock_millis_since_epoch_utc(real_time_clock_t *clock);

#endif // _TCLIB_UTILS_CLOCK_H
