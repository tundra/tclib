//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "utils/clock.hh"

using namespace tclib;

class SystemRealTimeClock : public RealTimeClock {
public:
  virtual NativeTime time_since_epoch_utc();

  // Returns the singleton system real time clock instance.
  static SystemRealTimeClock *instance() { return &kInstance; }

private:
  SystemRealTimeClock() { }

  // Static singleton instance.
  static SystemRealTimeClock kInstance;
};

SystemRealTimeClock SystemRealTimeClock::kInstance;

RealTimeClock *RealTimeClock::system() {
  return SystemRealTimeClock::instance();
}

NativeTime::NativeTime(platform_time_t platform) {
  time = platform;
}

NativeTime::NativeTime(native_time_t native) {
  time = native.time;
}

real_time_clock_t *real_time_clock_system() {
  return RealTimeClock::system();
}

native_time_t real_time_clock_time_since_epoch_utc(real_time_clock_t *clock) {
  return static_cast<RealTimeClock*>(clock)->time_since_epoch_utc();
}

uint64_t native_time_to_millis(native_time_t time) {
  return NativeTime(time).to_millis();
}

#ifdef IS_GCC
#  ifdef IS_MACH
#    include "clock-mach.cc"
#  else
#    include "clock-posix.cc"
#  endif
#endif

#ifdef IS_MSVC
#include "clock-msvc.cc"
#endif
