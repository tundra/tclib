//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "sync/worklist.h"
END_C_INCLUDES

#define kSize 100
#define kWidth 3

DECLARE_BOUNDED_BUFFER(kSize, kWidth);
DECLARE_WORKLIST(kSize, kWidth);

TEST(worklist, simple) {
  worklist_t(kSize, kWidth) worklist;
  ASSERT_TRUE(worklist_init(kSize, kWidth)(&worklist));
  ASSERT_TRUE(worklist_is_empty(kSize, kWidth)(&worklist));

  for (size_t i = 0; i < kSize; i++) {
    opaque_t elms[kWidth] = {u2o(i), u2o(i + 13), u2o(i + 17)};
    ASSERT_TRUE(worklist_schedule(kSize, kWidth)(&worklist, elms, kWidth, duration_unlimited()));
  }
  opaque_t elms[kWidth] = {opaque_null(), opaque_null(), opaque_null()};
  ASSERT_FALSE(worklist_schedule(kSize, kWidth)(&worklist, elms, kWidth, duration_seconds(0.01)));

  for (size_t i = 0; i < kSize; i++) {
    opaque_t elms[kWidth];
    ASSERT_TRUE(worklist_take(kSize, kWidth)(&worklist, elms, kWidth, duration_instant()));
    ASSERT_EQ(i, o2u(elms[0]));
    ASSERT_EQ(i + 13, o2u(elms[1]));
    ASSERT_EQ(i + 17, o2u(elms[2]));
  }
  ASSERT_FALSE(worklist_take(kSize, kWidth)(&worklist, NULL, kWidth, duration_instant()));

  worklist_dispose(kSize, kWidth)(&worklist);
}

IMPLEMENT_BOUNDED_BUFFER(kSize, kWidth);
IMPLEMENT_WORKLIST(kSize, kWidth);
