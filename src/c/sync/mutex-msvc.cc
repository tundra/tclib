//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

bool NativeMutex::platform_initialize() {
  handle_t result = CreateMutex(
      NULL,  // lpMutexAttributes
      false, // bInitialOwner
      NULL); // lpName
  if (result == NULL) {
    WARN("Call to CreateMutex failed: %i", GetLastError());
    return false;
  }
  mutex = result;
  return true;
}

bool NativeMutex::platform_dispose() {
  bool result = CloseHandle(mutex);
  if (result) {
    mutex = INVALID_HANDLE_VALUE;
  } else {
    WARN("Call to CloseHandle failed: %i", GetLastError());
  }
  return result;
}

bool NativeMutex::lock() {
  dword_t result = WaitForSingleObject(mutex, INFINITE);
  if (result == WAIT_OBJECT_0)
    return true;
  if (result == WAIT_FAILED)
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
  return false;
}

bool NativeMutex::try_lock() {
  dword_t result = WaitForSingleObject(mutex, 0);
  if (result == WAIT_OBJECT_0)
    return true;
  if (result == WAIT_FAILED)
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
  return false;
}

bool NativeMutex::unlock() {
  bool result = ReleaseMutex(mutex);
  if (result)
    return true;
  WARN("Call to ReleaseMutex failed: %i", GetLastError());
  return false;
}
