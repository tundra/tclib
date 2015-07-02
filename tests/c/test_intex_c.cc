//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "sync/intex.h"
END_C_INCLUDES

TEST(intex_c, simple) {
  intex_t intex;
  intex_construct(&intex, 18);
  ASSERT_TRUE(intex_initialize(&intex));
  ASSERT_FALSE(intex_lock_when_equal(&intex, 17, duration_instant()));
  ASSERT_FALSE(intex_lock_when_equal(&intex, 19, duration_instant()));
  ASSERT_TRUE(intex_lock_when_equal(&intex, 18, duration_instant()));
  ASSERT_TRUE(intex_unlock(&intex));
  ASSERT_FALSE(intex_lock_when_less(&intex, 18, duration_instant()));
  ASSERT_TRUE(intex_lock_when_less(&intex, 19, duration_instant()));
  ASSERT_TRUE(intex_unlock(&intex));
  ASSERT_FALSE(intex_lock_when_greater(&intex, 18, duration_instant()));
  ASSERT_TRUE(intex_lock_when_greater(&intex, 17, duration_instant()));
  ASSERT_TRUE(intex_unlock(&intex));
  intex_dispose(&intex);
}
