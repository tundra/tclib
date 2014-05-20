//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// This is just idiotic.
#pragma warning(push, 0)
#include <windows.h>
#pragma warning(pop)

double get_current_time_seconds() {
  SYSTEMTIME time;
  GetSystemTime(&time);
  WORD hr = time.wHour;
  WORD mn = time.wMinute;
  WORD sc = time.wSecond;
  WORD ms = time.wMilliseconds;
  return (60.0 * 60.0 * hr) + (60.0 * mn) + sc + (ms / 1000.0);
}
