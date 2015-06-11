//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_UTILS_CLOCK_HH
#define _TCLIB_UTILS_CLOCK_HH

#include "c/stdc.h"

#include "utils/duration.hh"

BEGIN_C_INCLUDES
#include "utils/clock.h"
END_C_INCLUDES

struct real_time_clock_t { };

namespace tclib {

class NativeTime : public native_time_t {
public:
  NativeTime(platform_time_t platform);

  NativeTime(native_time_t native);

  // Returns this time converted to the type used by this platform.
  const platform_time_t &to_platform() { return time; }

  // Wonderfully, mach uses two different kinds of time: mach_timespec and
  // timespec. In case we definitely, positively, need the plain timespec this
  // is what you should call instead of to_platform.
  ONLY_GCC(struct timespec to_posix();)

  // Returns the number of milliseconds represented by the given native time
  // object.
  uint64_t to_millis();

  // Returns a new native time object which is this one advanced by the given
  // duration.
  NativeTime operator+(duration_t duration);

  // Returns a native time representing zero.
  static NativeTime zero();
};

// A clock is a service for telling what the current system time is. A clock is
// not necessarily connected to the concurrency primitives that are influenced
// by real time (timeouts etc.).
class RealTimeClock : public real_time_clock_t {
public:
  virtual ~RealTimeClock() { }

  // Returns the time elapsed since unix epoch.
  virtual NativeTime time_since_epoch_utc() = 0;

  // Returns a clock that represents system time.
  static RealTimeClock *system();
};

} // namespace tclib

#endif // _TCLIB_UTILS_CLOCK_HH
