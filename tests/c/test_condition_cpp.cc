//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "sync/condition.hh"
#include "sync/mutex.hh"
#include "sync/semaphore.hh"
#include "sync/thread.hh"
#include "test/unittest.hh"

using namespace tclib;

TEST(condition_cpp, simple) {
  NativeCondition condvar;
  ASSERT_TRUE(condvar.initialize());
}

#if defined(IS_MSVC)
#  include "c/winhdr.h"
#endif

TEST(condition_cpp, msvc_sizes) {
#if defined(IS_MSVC)
  // If this fails it should be easy to fix, just bump up the size of the
  // platform condition type.
  ASSERT_TRUE(sizeof(platform_condition_t) >= sizeof(CONDITION_VARIABLE));
#endif
}
