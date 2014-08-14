//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "mutex.hh"
#include <new>

using namespace tclib;

#ifdef IS_GCC
#include "mutex-posix.cc"
#endif

#ifdef IS_MSVC
#include "mutex-msvc.cc"
#endif

NativeMutex::NativeMutex()
  : data_(NULL) { }

NativeMutex::~NativeMutex() {
  if (data_ != NULL) {
    data_->~Data();
    data_ = NULL;
  }
}

bool NativeMutex::initialize() {
  if (kMaxDataSize < sizeof(Data))
    return false;
  data_ = new (data_memory_) Data();
  return data_->initialize();
}

bool NativeMutex::lock() {
  return data_->lock();
}

bool NativeMutex::try_lock() {
  return data_->try_lock();
}

bool NativeMutex::unlock() {
  return data_->unlock();
}

size_t NativeMutex::get_data_size() {
  return sizeof(Data);
}
