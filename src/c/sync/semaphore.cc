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
  , data_(NULL) { }

NativeSemaphore::NativeSemaphore(uint32_t initial_count)
  : initial_count_(initial_count)
  , data_(NULL) { }

NativeSemaphore::~NativeSemaphore() {
  if (data_ != NULL) {
    data_->~Data();
    data_ = NULL;
  }
}

bool NativeSemaphore::initialize() {
  if (kMaxDataSize < sizeof(Data)) {
    WARN("Semaphore data size too small: %i < %i", kMaxDataSize, sizeof(Data));
    return false;
  }
  data_ = new (data_memory_) Data();
  return data_->initialize(initial_count_);
}

bool NativeSemaphore::acquire() {
  return data_->acquire();
}

bool NativeSemaphore::try_acquire() {
  return data_->try_acquire();
}

bool NativeSemaphore::release() {
  return data_->release();
}

size_t NativeSemaphore::get_data_size() {
  return sizeof(Data);
}
