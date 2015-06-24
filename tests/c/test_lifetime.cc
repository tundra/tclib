//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "utils/lifetime.h"
END_C_INCLUDES

static opaque_t add_to_count(opaque_t o_count) {
  int *count = (int*) o2p(o_count);
  (*count) += 1;
  return o0();
}

TEST(lifetime, defawlt) {
  int count = 0;
  lifetime_t lifetime;
  lifetime_begin(&lifetime);
  lifetime_atexit(&lifetime, nullary_callback_new_1(add_to_count, p2o(&count)));
  lifetime_atexit(&lifetime, nullary_callback_new_1(add_to_count, p2o(&count)));
  lifetime_atexit(&lifetime, nullary_callback_new_1(add_to_count, p2o(&count)));
  ASSERT_EQ(0, count);
  lifetime_end(&lifetime);
  ASSERT_EQ(3, count);
}
