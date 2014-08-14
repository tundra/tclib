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
  if (result == NULL)
    return false;
  mutex_ = result;
  return true;
}

NativeMutex::Data::~Data() {
  if (mutex_ != INVALID_HANDLE_VALUE)
    CloseHandle(mutex_);
}

bool NativeMutex::Data::lock() {
  return WaitForSingleObject(mutex_, INFINITE) == WAIT_OBJECT_0;
}

bool NativeMutex::Data::try_lock() {
  return WaitForSingleObject(mutex_, 0) == WAIT_OBJECT_0;
}

bool NativeMutex::Data::unlock() {
  return ReleaseMutex(mutex_) != 0;
}
