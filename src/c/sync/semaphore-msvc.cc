//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "winhdr.h"

class NativeSemaphore::Data {
public:
  Data();
  ~Data();
  bool initialize(uint32_t initial_count);
  bool acquire();
  bool try_acquire();
  bool release();
private:
  handle_t sema_;
};

NativeSemaphore::Data::Data()
  : sema_(INVALID_HANDLE_VALUE) { }

bool NativeSemaphore::Data::initialize(uint32_t initial_count) {
  handle_t result = CreateSemaphore(
      NULL, // lpSemaphoreAttributes
      initial_count, // lInitialCount
      0x7FFFFFFF, // lMaximumCount
      NULL); // lpName
  if (result == NULL) {
    WARN("Call to CreateSemaphore failed: %i", GetLastError());
    return false;
  }
  sema_ = result;
  return true;
}

NativeSemaphore::Data::~Data() {
  if (sema_ != INVALID_HANDLE_VALUE) {
    if (!CloseHandle(sema_))
      WARN("Call to CloseHandle failed: %i", GetLastError());
  }
}

bool NativeSemaphore::Data::acquire() {
  dword_t result = WaitForSingleObject(sema_, INFINITE);
  if (result == WAIT_OBJECT_0)
    return true;
  if (result == WAIT_FAILED)
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
  return false;
}

bool NativeSemaphore::Data::try_acquire() {
  dword_t result = WaitForSingleObject(sema_, 0);
  if (result == WAIT_OBJECT_0)
    return true;
  if (result == WAIT_FAILED)
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
  return false;
}

bool NativeSemaphore::Data::release() {
  bool result = ReleaseSemaphore(
      sema_, // hSemaphore
      1,     // lReleaseCount
      NULL); // lpPreviousCount
  if (result)
    return true;
  WARN("Call to ReleaseSemaphore failed: %i", GetLastError());
  return false;
}
