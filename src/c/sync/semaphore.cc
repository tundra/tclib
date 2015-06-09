//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "sync/semaphore.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
#include "sync/semaphore.h"
END_C_INCLUDES

#include <new>

using namespace tclib;

#ifdef IS_GCC
#  ifdef IS_MACH
#    include "semaphore-mach.cc"
#  else
#    include "semaphore-posix.cc"
#  endif
#endif

#ifdef IS_MSVC
#  include "semaphore-msvc.cc"
#endif

NativeSemaphore::NativeSemaphore() {
  initial_count = 1;
  is_initialized = false;
#ifdef kNativeSemaphoreInit
  native_semaphore_t init = kNativeSemaphoreInit;
  sema = init;
#endif
}

NativeSemaphore::NativeSemaphore(uint32_t count) {
  initial_count = count;
  is_initialized = false;
#ifdef kNativeSemaphoreInit
  native_semaphore_t init = kNativeSemaphoreInit;
  sema = init;
#endif
}

NativeSemaphore::~NativeSemaphore() {
  if (!is_initialized)
    return;
  is_initialized = false;
  platform_dispose();
}

bool NativeSemaphore::initialize() {
  if (!is_initialized)
    is_initialized = platform_initialize();
  return is_initialized;
}

bool NativeSemaphore::try_acquire() {
  return acquire(Duration::instant());
}

void native_semaphore_construct(native_semaphore_t *sema) {
  new (sema) NativeSemaphore();
}

void native_semaphore_construct_with_count(native_semaphore_t *sema,
    uint32_t initial_count) {
  new (sema) NativeSemaphore(initial_count);
}

void native_semaphore_set_initial_count(native_semaphore_t *sema, uint32_t value) {
  static_cast<NativeSemaphore*>(sema)->set_initial_count(value);
}

bool native_semaphore_initialize(native_semaphore_t *sema) {
  return static_cast<NativeSemaphore*>(sema)->initialize();
}

bool native_semaphore_acquire(native_semaphore_t *sema, duration_t timeout) {
  return static_cast<NativeSemaphore*>(sema)->acquire(timeout);
}

bool native_semaphore_try_acquire(native_semaphore_t *sema) {
  return static_cast<NativeSemaphore*>(sema)->try_acquire();
}

bool native_semaphore_release(native_semaphore_t *sema) {
  return static_cast<NativeSemaphore*>(sema)->release();
}

void native_semaphore_dispose(native_semaphore_t *sema) {
  static_cast<NativeSemaphore*>(sema)->~NativeSemaphore();
}
