//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "utils/macro-inl.h"
END_C_INCLUDES

// Record that this macro has been invoked with the given argument.
#define RECORD(N) record[count++] = N;

TEST(macro, for_each_va_arg) {
  int count = 0;
  int record[8];

  FOR_EACH_VA_ARG(RECORD, 1, 4, 6, 9, 34, 54, 2, 3);
  ASSERT_EQ(8, count);
  ASSERT_EQ(record[0], 1);
  ASSERT_EQ(record[1], 4);
  ASSERT_EQ(record[2], 6);
  ASSERT_EQ(record[3], 9);
  ASSERT_EQ(record[4], 34);
  ASSERT_EQ(record[5], 54);
  ASSERT_EQ(record[6], 2);
  ASSERT_EQ(record[7], 3);

  count = 0;
  FOR_EACH_VA_ARG(RECORD, 6, 5, 4);
  ASSERT_EQ(3, count);
  ASSERT_EQ(record[0], 6);
  ASSERT_EQ(record[1], 5);
  ASSERT_EQ(record[2], 4);
}

TEST(macro, va_argc) {
  ASSERT_EQ(1, VA_ARGC(a));
  ASSERT_EQ(2, VA_ARGC(a, b));
  ASSERT_EQ(3, VA_ARGC(a, b, c));
  ASSERT_EQ(4, VA_ARGC(a, b, c, d));
  ASSERT_EQ(5, VA_ARGC(a, b, c, d, e));
  ASSERT_EQ(6, VA_ARGC(a, b, c, d, e, f));
  ASSERT_EQ(7, VA_ARGC(a, b, c, d, e, f, g));
  ASSERT_EQ(8, VA_ARGC(a, b, c, d, e, f, g, h));
}
