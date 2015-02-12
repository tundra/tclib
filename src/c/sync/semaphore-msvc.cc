//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

bool NativeSemaphore::platform_initialize() {
  handle_t result = CreateSemaphore(
      NULL, // lpSemaphoreAttributes
      initial_count, // lInitialCount
      0x7FFFFFFF, // lMaximumCount
      NULL); // lpName
  if (result == NULL) {
    WARN("Call to CreateSemaphore failed: %i", GetLastError());
    return false;
  }
  sema = result;
  return true;
}

bool NativeSemaphore::platform_dispose() {
  if (sema != INVALID_HANDLE_VALUE) {
    if (!CloseHandle(sema)) {
      WARN("Call to CloseHandle failed: %i", GetLastError());
      return false;
    }
  }
  return true;
}

bool NativeSemaphore::acquire(duration_t timeout) {
  dword_t millis = duration_is_unlimited(timeout)
    ? INFINITE
    : duration_to_millis(timeout);
  dword_t result = WaitForSingleObject(sema, millis);
  if (result == WAIT_OBJECT_0)
    return true;
  if (result == WAIT_FAILED)
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
  return false;
}

bool NativeSemaphore::try_acquire() {
  dword_t result = WaitForSingleObject(sema, 0);
  if (result == WAIT_OBJECT_0)
    return true;
  if (result == WAIT_FAILED)
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
  return false;
}

bool NativeSemaphore::release() {
  bool result = ReleaseSemaphore(
      sema,  // hSemaphore
      1,     // lReleaseCount
      NULL); // lpPreviousCount
  if (result)
    return true;
  WARN("Call to ReleaseSemaphore failed: %i", GetLastError());
  return false;
}
