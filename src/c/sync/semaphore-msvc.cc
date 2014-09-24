//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "winhdr.h"

bool NativeSemaphore::platform_initialize() {
  handle_t result = CreateSemaphore(
      NULL, // lpSemaphoreAttributes
      initial_count_, // lInitialCount
      0x7FFFFFFF, // lMaximumCount
      NULL); // lpName
  if (result == NULL) {
    WARN("Call to CreateSemaphore failed: %i", GetLastError());
    return false;
  }
  sema_ = result;
  return true;
}

bool NativeSemaphore::platform_dispose() {
  if (sema_ != INVALID_HANDLE_VALUE) {
    if (!CloseHandle(sema_)) {
      WARN("Call to CloseHandle failed: %i", GetLastError());
      return false;
    }
  }
  return true;
}

bool NativeSemaphore::acquire() {
  dword_t result = WaitForSingleObject(sema_, INFINITE);
  if (result == WAIT_OBJECT_0)
    return true;
  if (result == WAIT_FAILED)
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
  return false;
}

bool NativeSemaphore::try_acquire() {
  dword_t result = WaitForSingleObject(sema_, 0);
  if (result == WAIT_OBJECT_0)
    return true;
  if (result == WAIT_FAILED)
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
  return false;
}

bool NativeSemaphore::release() {
  bool result = ReleaseSemaphore(
      sema_, // hSemaphore
      1,     // lReleaseCount
      NULL); // lpPreviousCount
  if (result)
    return true;
  WARN("Call to ReleaseSemaphore failed: %i", GetLastError());
  return false;
}
