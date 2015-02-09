//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// This is just idiotic.
#include "winhdr.h"

double get_current_time_seconds() {
  SYSTEMTIME time;
  GetSystemTime(&time);
  word_t hr = time.wHour;
  word_t mn = time.wMinute;
  word_t sc = time.wSecond;
  word_t ms = time.wMilliseconds;
  return (60.0 * 60.0 * hr) + (60.0 * mn) + sc + (ms / 1000.0);
}
