//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "sync/process.hh"

using namespace tclib;

TEST(process_cpp, exec_missing) {
  NativeProcess process;
  ASSERT_TRUE(process.start("test_process_cpp_exec_fail_missing_executable", 0, NULL));
  ASSERT_TRUE(process.wait());
  ASSERT_TRUE(process.exit_code() != 0);
}

#if defined(IS_MSVC)
#  include "c/winhdr.h"
#endif

TEST(process_cpp, msvc_sizes) {
#if defined(IS_MSVC)
  // If this fails it should be easy to fix, just bump up the size of the
  // platform process type.
  ASSERT_REL(sizeof(platform_process_t), >=, sizeof(PROCESS_INFORMATION));
#endif
}
