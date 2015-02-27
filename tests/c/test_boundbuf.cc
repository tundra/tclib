//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "utils/boundbuf.h"
END_C_INCLUDES

TEST(boundbuf, sizes) {
  // Size-1 is the case declared in the type.
  ASSERT_EQ(sizeof(bounded_buffer_t), BOUNDED_BUFFER_SIZE(1));
}

TEST(boundbuf, simple) {
  uint8_t buf[BOUNDED_BUFFER_SIZE(100)];
  bounded_buffer_init(buf, sizeof(buf), 100);
  for (size_t i = 0; i < 100; i++)
    ASSERT_TRUE(bounded_buffer_try_offer(buf, u2o(i)));
  ASSERT_FALSE(bounded_buffer_try_offer(buf, u2o(100)));
  for (size_t i = 0; i < 100; i++) {
    opaque_t val = opaque_null();
    ASSERT_TRUE(bounded_buffer_try_take(buf, &val));
    ASSERT_EQ(i, o2u(val));
  }
  ASSERT_FALSE(bounded_buffer_try_take(buf, NULL));
  // Unaligned adding and removing.
  size_t next_expected = 1001;
  size_t next_to_add = 1001;
  size_t occupancy = 0;
  while (occupancy <= 93) {
    for (size_t io = 0; io < 7; io++) {
      ASSERT_TRUE(bounded_buffer_try_offer(buf, u2o(next_to_add)));
      next_to_add++;
      occupancy++;
    }
    for (size_t it = 0; it < 5; it++) {
      opaque_t value;
      ASSERT_TRUE(bounded_buffer_try_take(buf, &value));
      ASSERT_EQ(next_expected, o2u(value));
      next_expected++;
      occupancy--;
    }
  }

}
