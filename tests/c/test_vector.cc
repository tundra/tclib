//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "utils/vector.h"
END_C_INCLUDES

TEST(vector, voidp_simple) {
  voidp_vector_t vector;
  voidp_vector_init(&vector);
  ASSERT_EQ(0, voidp_vector_size(&vector));
  void *a = (void*) 10;
  void *b = (void*) 20;
  voidp_vector_push_back(&vector, a);
  ASSERT_EQ(1, voidp_vector_size(&vector));
  ASSERT_PTREQ(a, voidp_vector_get(&vector, 0));
  voidp_vector_push_back(&vector, b);
  ASSERT_EQ(2, voidp_vector_size(&vector));
  ASSERT_PTREQ(a, voidp_vector_get(&vector, 0));
  ASSERT_PTREQ(b, voidp_vector_get(&vector, 1));
  voidp_vector_dispose(&vector);
}
