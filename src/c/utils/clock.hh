//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_UTILS_CLOCK_HH
#define _TCLIB_UTILS_CLOCK_HH

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "utils/clock.h"
END_C_INCLUDES

struct real_time_clock_t { };

namespace tclib {

// A clock is a service for telling what the current system time is. A clock is
// not necessarily connected to the concurrency primitives that are influenced
// by real time (timeouts etc.).
class RealTimeClock : public real_time_clock_t {
public:
  virtual ~RealTimeClock() { }

  // Returns the number of milliseconds since unix epoch.
  virtual uint64_t millis_since_epoch_utc() = 0;

  // Returns a clock that represents system time.
  static RealTimeClock *system();
};

} // namespace tclib

#endif // _TCLIB_UTILS_CLOCK_HH
