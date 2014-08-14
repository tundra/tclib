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
  if (result == NULL)
    return false;
  sema_ = result;
  return true;
}

NativeSemaphore::Data::~Data() {
  if (sema_ != INVALID_HANDLE_VALUE)
    CloseHandle(sema_);
}

bool NativeSemaphore::Data::acquire() {
  return WaitForSingleObject(sema_, INFINITE) == WAIT_OBJECT_0;
}

bool NativeSemaphore::Data::try_acquire() {
  return WaitForSingleObject(sema_, 0) == WAIT_OBJECT_0;
}

bool NativeSemaphore::Data::release() {
  return ReleaseSemaphore(
      sema_, // hSemaphore
      1,     // lReleaseCount
      NULL); // lpPreviousCount
}
