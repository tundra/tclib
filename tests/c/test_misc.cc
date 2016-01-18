//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "utils/misc-inl.h"
END_C_INCLUDES

TEST(misc, power_of_2) {
  ASSERT_TRUE(is_power_of_2(0));
  ASSERT_TRUE(is_power_of_2(1));
  ASSERT_TRUE(is_power_of_2(2));
  ASSERT_FALSE(is_power_of_2(3));
  ASSERT_TRUE(is_power_of_2(4));
  ASSERT_FALSE(is_power_of_2(5));
  ASSERT_FALSE(is_power_of_2(6));
  ASSERT_FALSE(is_power_of_2(7));
  ASSERT_TRUE(is_power_of_2(8));
  ASSERT_FALSE(is_power_of_2(65536 - 1));
  ASSERT_TRUE(is_power_of_2(65536));
  ASSERT_FALSE(is_power_of_2(65536 + 1));
  ASSERT_FALSE(is_power_of_2((1ULL << 32) - 1ULL));
  ASSERT_TRUE(is_power_of_2(1ULL << 32));
  ASSERT_FALSE(is_power_of_2((1ULL << 32) + 1ULL));
  ASSERT_FALSE(is_power_of_2((1ULL << 48) - 1ULL));
  ASSERT_TRUE(is_power_of_2(1ULL << 48));
  ASSERT_FALSE(is_power_of_2((1ULL << 48) + 1ULL));
}
