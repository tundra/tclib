//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "sync/condition.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
#include "sync/condition.h"
END_C_INCLUDES

using namespace tclib;

#ifdef IS_GCC
#  include "condition-posix.cc"
#endif

#ifdef IS_MSVC
#  include "condition-msvc.cc"
#endif

NativeCondition::NativeCondition() {
  is_initialized = false;
}

NativeCondition::~NativeCondition() {
  if (!is_initialized)
    return;
  is_initialized = false;
  platform_dispose();
}

bool NativeCondition::initialize() {
  if (!is_initialized)
    is_initialized = platform_initialize();
  return is_initialized;
}
