//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "sync/process.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
END_C_INCLUDES

using namespace tclib;

NativeProcess::NativeProcess() {
#if defined(kPlatformProcessInit)
  process = kPlatformProcessInit;
#endif
  state = nsInitial;
  result = -1;
  platform_initialize();
}

NativeProcess::~NativeProcess() {
  if (state != nsInitial)
    platform_dispose();
}

#ifdef IS_GCC
#include "process-posix.cc"
#endif

#ifdef IS_MSVC
#include "process-msvc.cc"
#endif
