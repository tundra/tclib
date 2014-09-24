//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "mutex.hh"

BEGIN_C_INCLUDES
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
