//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

bool NativeMutex::platform_initialize() {
  InitializeCriticalSection(get_platform_mutex(this));
  return true;
}

bool NativeMutex::platform_dispose() {
  DeleteCriticalSection(get_platform_mutex(this));
  return true;
}

bool NativeMutex::lock(Duration timeout) {
  CRITICAL_SECTION *mutex = get_platform_mutex(this);
  if (timeout.is_unlimited()) {
    EnterCriticalSection(mutex);
    return true;
  } else if (timeout.is_instant()) {
    return TryEnterCriticalSection(mutex);
  } else {
    CHECK_TRUE("nontrivial mutex timeout", false);
    return false;
  }
}

bool NativeMutex::unlock() {
  LeaveCriticalSection(get_platform_mutex(this));
  return true;
}
