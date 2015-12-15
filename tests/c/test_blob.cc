//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

#include "utils/blob.hh"

using namespace tclib;

TEST(blob, constr) {
  int i = 0;
  Blob b(&i, 100);
  ASSERT_PTREQ(&i, b.start());
  ASSERT_EQ(100, b.size());
  ASSERT_PTREQ(reinterpret_cast<byte_t*>(&i) + 100, b.end());
}
