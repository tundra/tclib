//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "sync/mutex.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
#include "sync/mutex.h"
END_C_INCLUDES

#include <new>

using namespace tclib;

#ifdef IS_GCC
#include "mutex-posix.cc"
#endif

#ifdef IS_MSVC
#include "mutex-msvc.cc"
#endif

NativeMutex::NativeMutex()
  : is_initialized_(false) {
  platform_mutex_t init = kPlatformMutexInit;
  mutex_ = init;
}

NativeMutex::~NativeMutex() {
  if (!is_initialized_)
    return;
  is_initialized_ = false;
  platform_dispose();
}

bool NativeMutex::initialize() {
  if (!is_initialized_)
    is_initialized_ = platform_initialize();
  return is_initialized_;
}

native_mutex_t *new_native_mutex() {
  return reinterpret_cast<native_mutex_t*>(new NativeMutex());
}

void native_mutex_dispose(native_mutex_t *mutex) {
  delete reinterpret_cast<NativeMutex*>(mutex);
}

bool native_mutex_initialize(native_mutex_t *mutex) {
  return reinterpret_cast<NativeMutex*>(mutex)->initialize();
}

bool native_mutex_lock(native_mutex_t *mutex) {
  return reinterpret_cast<NativeMutex*>(mutex)->lock();
}

bool native_mutex_try_lock(native_mutex_t *mutex) {
  return reinterpret_cast<NativeMutex*>(mutex)->try_lock();
}

bool native_mutex_unlock(native_mutex_t *mutex) {
  return reinterpret_cast<NativeMutex*>(mutex)->unlock();
}
