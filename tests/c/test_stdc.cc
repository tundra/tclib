//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

TEST(stdc, int_types) {
  ASSERT_EQ(1, sizeof(uint8_t));
  ASSERT_EQ(1, sizeof(int8_t));
  ASSERT_EQ(2, sizeof(uint16_t));
  ASSERT_EQ(2, sizeof(int16_t));
  ASSERT_EQ(4, sizeof(uint32_t));
  ASSERT_EQ(4, sizeof(int32_t));
  ASSERT_EQ(8, sizeof(uint64_t));
  ASSERT_EQ(8, sizeof(int64_t));
}

TEST(stdc, pointer_size) {
#ifdef IS_32_BIT
  ASSERT_EQ(4, sizeof(void*));
#endif

#ifdef IS_64_BIT
  ASSERT_EQ(8, sizeof(void*));
#endif

  ASSERT_EQ(IF_32_BIT(4, 8), sizeof(void*));
  ASSERT_EQ(IF_64_BIT(8, 4), sizeof(void*));
}
