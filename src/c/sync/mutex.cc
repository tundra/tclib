//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "sync/mutex.hh"

BEGIN_C_INCLUDES
#include "sync/mutex.h"
#include "utils/alloc.h"
#include "utils/log.h"
END_C_INCLUDES

#include <new>

using namespace tclib;

#ifdef IS_GCC
#include "mutex-posix.cc"
#endif

#ifdef IS_MSVC
#include "mutex-msvc.cc"
#endif

NativeMutex::NativeMutex() {
  is_initialized = false;
#if defined(kPlatformMutexInit)
  platform_mutex_t init = kPlatformMutexInit;
  mutex = init;
#endif
}

NativeMutex::~NativeMutex() {
  if (!is_initialized)
    return;
  is_initialized = false;
  platform_dispose();
  struct_zero_fill(*this);
}

fat_bool_t NativeMutex::initialize() {
  if (!is_initialized) {
    F_TRY(platform_initialize());
    is_initialized = true;
  }
  return F_TRUE;
}

fat_bool_t NativeMutex::try_lock() {
  return lock(Duration::instant());
}

void native_mutex_construct(native_mutex_t *mutex) {
  new (mutex) NativeMutex();
}

void native_mutex_dispose(native_mutex_t *mutex) {
  static_cast<NativeMutex*>(mutex)->~NativeMutex();
}

bool native_mutex_initialize(native_mutex_t *mutex) {
  return static_cast<NativeMutex*>(mutex)->initialize();
}

bool native_mutex_lock(native_mutex_t *mutex) {
  return static_cast<NativeMutex*>(mutex)->lock();
}

bool native_mutex_try_lock(native_mutex_t *mutex) {
  return static_cast<NativeMutex*>(mutex)->try_lock();
}

bool native_mutex_unlock(native_mutex_t *mutex) {
  return static_cast<NativeMutex*>(mutex)->unlock();
}

bool native_mutex_checks_consistency() {
  return NativeMutex::checks_consistency();
}

