//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <time.h>

NativeTime SystemRealTimeClock::time_since_epoch_utc() {
  platform_time_t spec;
  clock_gettime(CLOCK_REALTIME, &spec);
  return spec;
}

uint64_t NativeTime::to_millis() {
  return static_cast<uint64_t>((static_cast<double>(time.tv_sec) * 1000.0) + (static_cast<double>(time.tv_nsec) / 1000000.0));
}

NativeTime NativeTime::zero() {
  platform_time_t time;
  time.tv_nsec = 0;
  time.tv_sec = 0;
  return time;
}

NativeTime NativeTime::operator+(duration_t duration) {
  uint64_t sec = time.tv_sec;
  uint64_t nsec = time.tv_nsec;
  duration_add_to_timespec(duration, &sec, &nsec);
  platform_time_t result;
  result.tv_sec = static_cast<time_t>(sec);
  result.tv_nsec = static_cast<long>(nsec);
  return result;
}

platform_time_t NativeTime::to_posix() {
  return to_platform();
}

struct timeval duration_to_timeval(duration_t duration) {
  uint64_t sec = 0;
  uint64_t usec = 0;
  duration_add_to_timeval(duration, &sec, &usec);
  struct timeval result;
  result.tv_sec = static_cast<time_t>(sec);
  result.tv_usec = static_cast<suseconds_t>(usec);
  return result;
}
