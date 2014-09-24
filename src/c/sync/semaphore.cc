//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "semaphore.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
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

NativeSemaphore::NativeSemaphore()
  : initial_count_(1)
  , is_initialized_(false) {
#ifdef kNativeSemaphoreInit
  native_semaphore_t init = kNativeSemaphoreInit;
  sema_ = init;
#endif
}

NativeSemaphore::NativeSemaphore(uint32_t initial_count)
  : initial_count_(initial_count)
  , is_initialized_(false) { }

NativeSemaphore::~NativeSemaphore() {
  if (!is_initialized_)
    return;
  is_initialized_ = false;
  platform_dispose();
}

bool NativeSemaphore::initialize() {
  if (!is_initialized_)
    is_initialized_ = platform_initialize();
  return is_initialized_;
}
