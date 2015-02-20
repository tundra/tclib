//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

uint64_t SystemRealTimeClock::millis_since_epoch_utc() {
  SYSTEMTIME time;
  GetSystemTime(&time);
  uint64_t hr = time.wHour;
  uint64_t mn = time.wMinute + (60 * hr);
  uint64_t sc = time.wSecond + (60 * mn);
  uint64_t ms = time.wMilliseconds + (1000 * sc);
  return ms;
}
