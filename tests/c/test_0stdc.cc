//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "c/stdc-inl.h"
END_C_INCLUDES

#include <math.h>

TEST(0stdc, int_types) {
  ASSERT_EQ(1, sizeof(uint8_t));
  ASSERT_EQ(1, sizeof(int8_t));
  ASSERT_EQ(2, sizeof(uint16_t));
  ASSERT_EQ(2, sizeof(int16_t));
  ASSERT_EQ(4, sizeof(uint32_t));
  ASSERT_EQ(4, sizeof(int32_t));
  ASSERT_EQ(8, sizeof(uint64_t));
  ASSERT_EQ(8, sizeof(int64_t));
}

TEST(0stdc, pointer_size) {
#ifdef IS_32_BIT
  ASSERT_EQ(4, sizeof(void*));
#endif

#ifdef IS_64_BIT
  ASSERT_EQ(8, sizeof(void*));
#endif

  ASSERT_EQ(IF_32_BIT(4, 8), sizeof(void*));
  ASSERT_EQ(IF_64_BIT(8, 4), sizeof(void*));
}

int always_inline foo() {
  return 10;
}

always_inline int bar() {
  return 11;
}

TEST(0stdc, always_inlines) {
  ASSERT_EQ(10, foo());
  ASSERT_EQ(11, bar());
}

TEST(0stdc, format) {
  char buf[256];
  int64_t i64 = 100;
  sprintf(buf, "%" PRIi64, i64);
  sprintf(buf, "%" PRIx64, i64);
}

TEST(0stdc, infinities) {
  float ifi = kFloatInfinity;
  float nifi = -kFloatInfinity;
  float nan = NAN;
  ASSERT_FALSE(isfinite(ifi));
  ASSERT_FALSE(isfinite(nifi));
  ASSERT_FALSE(isfinite(nan));
  ASSERT_TRUE(isnan(nan));
}
