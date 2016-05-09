//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

fat_bool_t NativeSemaphore::platform_initialize() {
  handle_t result = CreateSemaphore(
      NULL, // lpSemaphoreAttributes
      initial_count, // lInitialCount
      0x7FFFFFFF, // lMaximumCount
      NULL); // lpName
  if (result == NULL) {
    WARN("Call to CreateSemaphore failed: %i", GetLastError());
    return F_FALSE;
  }
  sema = result;
  return F_TRUE;
}

fat_bool_t NativeSemaphore::platform_dispose() {
  if (sema != INVALID_HANDLE_VALUE) {
    if (!CloseHandle(sema)) {
      WARN("Call to CloseHandle failed: %i", GetLastError());
      return F_FALSE;
    }
  }
  return F_TRUE;
}

fat_bool_t NativeSemaphore::acquire(Duration timeout) {
  dword_t result = WaitForSingleObject(sema, timeout.to_winapi_millis());
  if (result == WAIT_OBJECT_0)
    return F_TRUE;
  if (result == WAIT_FAILED)
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
  return F_FALSE;
}

fat_bool_t NativeSemaphore::release() {
  bool result = ReleaseSemaphore(
      sema,  // hSemaphore
      1,     // lReleaseCount
      NULL); // lpPreviousCount
  if (result)
    return F_TRUE;
  WARN("Call to ReleaseSemaphore failed: %i", GetLastError());
  return F_FALSE;
}
