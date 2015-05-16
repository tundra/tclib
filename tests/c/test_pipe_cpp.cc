//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "sync/pipe.hh"
#include "utils/string-inl.h"

using namespace tclib;

TEST(pipe_cpp, simple) {
  NativePipe pipe;
  ASSERT_TRUE(pipe.open(NativePipe::pfDefault));
  ASSERT_EQ(12, pipe.out()->printf("Hello, pipe!"));
  char buf[256];
  memset(buf, 0, sizeof(char) * 256);
  ASSERT_FALSE(pipe.in()->at_eof());
  ASSERT_EQ(12, pipe.in()->read_bytes(buf, 12));
  ASSERT_C_STREQ("Hello, pipe!", buf);
  ASSERT_FALSE(pipe.in()->at_eof());
  ASSERT_TRUE(pipe.out()->close());
  ASSERT_EQ(0, pipe.in()->read_bytes(buf, 256));
  ASSERT_TRUE(pipe.in()->at_eof());
}
