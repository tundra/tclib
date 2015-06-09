//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <mach/clock.h>
#include <mach/mach.h>

NativeTime SystemRealTimeClock::time_since_epoch_utc() {
  clock_serv_t clock_serv;
  mach_timespec_t spec;
  host_get_clock_service(mach_host_self(), CALENDAR_CLOCK, &clock_serv);
  clock_get_time(clock_serv, &spec);
  mach_port_deallocate(mach_task_self(), clock_serv);
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
  result.tv_sec = static_cast<unsigned int>(sec);
  result.tv_nsec = static_cast<clock_res_t>(nsec);
  return result;
}

struct timespec NativeTime::to_posix() {
  struct timespec spec;
  spec.tv_sec = time.tv_sec;
  spec.tv_nsec = time.tv_nsec;
  return spec;
}
