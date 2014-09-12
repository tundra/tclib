//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "winhdr.h"

class NativeMutex::Data {
public:
  Data();
  ~Data();
  bool initialize();
  bool lock();
  bool try_lock();
  bool unlock();
private:
  handle_t mutex_;
};

NativeMutex::Data::Data()
  : mutex_(INVALID_HANDLE_VALUE) { }

bool NativeMutex::Data::initialize() {
  handle_t result = CreateMutex(
      NULL,  // lpMutexAttributes
      false, // bInitialOwner
      NULL); // lpName
  if (result == NULL) {
    WARN("Call to CreateMutex failed: %i", GetLastError());
    return false;
  }
  mutex_ = result;
  return true;
}

NativeMutex::Data::~Data() {
  if (mutex_ != INVALID_HANDLE_VALUE) {
    if (!CloseHandle(mutex_))
      WARN("Call to CloseHandle failed: %i", GetLastError());
  }
}

bool NativeMutex::Data::lock() {
  dword_t result = WaitForSingleObject(mutex_, INFINITE);
  if (result == WAIT_OBJECT_0)
    return true;
  if (result == WAIT_FAILED)
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
  return false;
}

bool NativeMutex::Data::try_lock() {
  dword_t result = WaitForSingleObject(mutex_, 0);
  if (result == WAIT_OBJECT_0)
    return true;
  if (result == WAIT_FAILED)
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
  return false;
}

bool NativeMutex::Data::unlock() {
  bool result = ReleaseMutex(mutex_);
  if (result)
      return true;
  WARN("Call to ReleaseMutex failed: %i", GetLastError());
  return false;
}
