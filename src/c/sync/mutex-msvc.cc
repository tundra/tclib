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

bool NativeMutex::lock() {
  EnterCriticalSection(get_platform_mutex(this));
  return true;
}

bool NativeMutex::try_lock() {
  return TryEnterCriticalSection(get_platform_mutex(this));
}

bool NativeMutex::unlock() {
  LeaveCriticalSection(get_platform_mutex(this));
  return true;
}
