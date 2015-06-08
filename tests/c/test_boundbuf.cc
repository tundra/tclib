//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "utils/boundbuf.h"
END_C_INCLUDES

#define kBufSize 100

DECLARE_BOUNDED_BUFFER(kBufSize, 2);

TEST(boundbuf, simple) {
  bounded_buffer_t(kBufSize, 2) buf;
  bounded_buffer_init(kBufSize, 2)(&buf);
  for (size_t i = 0; i < kBufSize; i++) {
    opaque_t elms[2] = {u2o(i + 8), u2o(i * 7)};
    ASSERT_TRUE(bounded_buffer_try_offer(kBufSize, 2)(&buf, elms, 2));
  }
  opaque_t last_elms[2] = {u2o(0), u2o(0)};
  ASSERT_FALSE(bounded_buffer_try_offer(kBufSize, 2)(&buf, last_elms, 2));
  for (size_t i = 0; i < kBufSize; i++) {
    opaque_t vals[2];
    ASSERT_TRUE(bounded_buffer_try_take(kBufSize, 2)(&buf, vals, 2));
    ASSERT_EQ(i + 8, o2u(vals[0]));
    ASSERT_EQ(i * 7, o2u(vals[1]));
  }
  ASSERT_FALSE(bounded_buffer_try_take(kBufSize, 2)(&buf, NULL, 2));
  // Unaligned adding and removing.
  size_t next_expected = 1001;
  size_t next_to_add = 1001;
  size_t occupancy = 0;
  while (occupancy <= 93) {
    for (size_t io = 0; io < 7; io++) {
      opaque_t elms[2] = { u2o(next_to_add), u2o(next_to_add * 13) };
      ASSERT_TRUE(bounded_buffer_try_offer(kBufSize, 2)(&buf, elms, 2));
      next_to_add++;
      occupancy++;
    }
    for (size_t it = 0; it < 5; it++) {
      opaque_t values[2];
      ASSERT_TRUE(bounded_buffer_try_take(kBufSize, 2)(&buf, values, 2));
      ASSERT_EQ(next_expected, o2u(values[0]));
      ASSERT_EQ(next_expected * 13, o2u(values[1]));
      next_expected++;
      occupancy--;
    }
  }
}

IMPLEMENT_BOUNDED_BUFFER(kBufSize, 2);
