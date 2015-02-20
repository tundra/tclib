//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "utils/clock.hh"

using namespace tclib;

class SystemRealTimeClock : public RealTimeClock {
public:
  virtual uint64_t millis_since_epoch_utc();

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

real_time_clock_t *real_time_clock_system() {
  return RealTimeClock::system();
}

uint64_t real_time_clock_millis_since_epoch_utc(real_time_clock_t *clock) {
  return static_cast<RealTimeClock*>(clock)->millis_since_epoch_utc();
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
