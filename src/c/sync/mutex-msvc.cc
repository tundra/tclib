//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

fat_bool_t NativeMutex::platform_initialize() {
  InitializeCriticalSection(get_platform_mutex(this));
  return F_TRUE;
}

fat_bool_t NativeMutex::platform_dispose() {
  DeleteCriticalSection(get_platform_mutex(this));
  return F_TRUE;
}

fat_bool_t NativeMutex::lock(Duration timeout) {
  CRITICAL_SECTION *mutex = get_platform_mutex(this);
  if (timeout.is_unlimited()) {
    EnterCriticalSection(mutex);
    return F_TRUE;
  } else if (timeout.is_instant()) {
    return F_BOOL(TryEnterCriticalSection(mutex));
  } else {
    CHECK_TRUE("nontrivial mutex timeout", false);
    return F_FALSE;
  }
}

fat_bool_t NativeMutex::unlock() {
  LeaveCriticalSection(get_platform_mutex(this));
  return F_TRUE;
}
