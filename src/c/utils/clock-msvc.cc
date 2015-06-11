//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

#include "utils/duration.hh"

uint64_t NativeTime::to_millis() {
  return time;
}

NativeTime NativeTime::zero() {
  return 0;
}

NativeTime NativeTime::operator+(duration_t duration) {
  return time + duration_to_millis(duration);
}

NativeTime SystemRealTimeClock::time_since_epoch_utc() {
  SYSTEMTIME time;
  GetSystemTime(&time);
  uint64_t hr = time.wHour;
  uint64_t mn = time.wMinute + (60 * hr);
  uint64_t sc = time.wSecond + (60 * mn);
  uint64_t ms = time.wMilliseconds + (1000 * sc);
  return ms;
}

uint32_t Duration::to_winapi_millis() {
  return is_unlimited() ? INFINITE : to_millis();
}
