//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "utils/duration.h"
END_C_INCLUDES

TEST(duration, string_simple) {
  static const uint64_t kNsPerS = 1000000000;
  uint64_t secs = 0;
  uint64_t nanos = kNsPerS / 2;
  duration_add_to_timespec(duration_millis(500), &secs, &nanos);
  ASSERT_EQ(1, secs);
  ASSERT_EQ(0, nanos);
  duration_add_to_timespec(duration_millis(50), &secs, &nanos);
  ASSERT_EQ(1, secs);
  ASSERT_EQ(50000000, nanos);
  duration_add_to_timespec(duration_millis(200), &secs, &nanos);
  ASSERT_EQ(1, secs);
  ASSERT_EQ(250000000, nanos);
  duration_add_to_timespec(duration_seconds(200), &secs, &nanos);
  ASSERT_EQ(201, secs);
  ASSERT_EQ(250000000, nanos);
  duration_add_to_timespec(duration_millis(987), &secs, &nanos);
  ASSERT_EQ(202, secs);
  ASSERT_EQ(237000000, nanos);
}
