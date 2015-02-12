//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

static PCRITICAL_SECTION wincrit(NativeMutex *mutex) {
  // There should be code elsewhere that ensures that the mutex field is large
  // enough to hold a native critical section.
  return reinterpret_cast<PCRITICAL_SECTION>(&mutex->mutex);
}

bool NativeMutex::platform_initialize() {
  InitializeCriticalSection(wincrit(this));
  return true;
}

bool NativeMutex::platform_dispose() {
  DeleteCriticalSection(wincrit(this));
  return true;
}

bool NativeMutex::lock() {
  HEST("lock");
  EnterCriticalSection(wincrit(this));
  return true;
}

bool NativeMutex::try_lock() {
  HEST("try_lock");
  return TryEnterCriticalSection(wincrit(this));
}

bool NativeMutex::unlock() {
  HEST("unlock");
  LeaveCriticalSection(wincrit(this));
  return true;
}
