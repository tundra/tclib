//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "utils/opaque.h"
END_C_INCLUDES

TEST(opaque, size) {
  ASSERT_EQ(sizeof(uint64_t), sizeof(opaque_t));
}

TEST(opaque, simple) {
  int ref = 0;
  opaque_t opq = p2o(&ref);
  int *refp = (int*) o2p(opq);
  ASSERT_TRUE(refp == &ref);
  uint64_t encoded = o2u(opq);
  opaque_t new_opq = u2o(encoded);
  int *new_refp = (int*) o2p(new_opq);
  ASSERT_TRUE(new_refp == &ref);
  uint64_t new_encoded = o2u(new_opq);
  ASSERT_EQ(encoded, new_encoded);
}

static int i0 = 0;
static int i1 = 0;

TEST(opaque, comparison) {
  ASSERT_TRUE(opaque_same(p2o(&i0), p2o(&i0)));
  ASSERT_FALSE(opaque_same(p2o(&i0), p2o(&i1)));
  ASSERT_FALSE(opaque_same(p2o(&i1), p2o(&i0)));
  ASSERT_TRUE(opaque_same(p2o(&i1), p2o(&i1)));
  ASSERT_TRUE(opaque_same(p2o(NULL), p2o(NULL)));
  ASSERT_TRUE(opaque_same(u2o(100), u2o(100)));
  ASSERT_FALSE(opaque_same(u2o(100), u2o(200)));
  ASSERT_FALSE(opaque_same(u2o(200), u2o(100)));
  ASSERT_TRUE(opaque_same(u2o(200), u2o(200)));
  // These aren't strictly guaranteed to be true since the address of i0 could
  // be 100 but in practice that's extremely unlikely to happen.
  ASSERT_FALSE(opaque_same(u2o(100), p2o(&i0)));
  ASSERT_FALSE(opaque_same(p2o(&i0), u2o(100)));
}

TEST(opaque, null) {
  ASSERT_TRUE(opaque_is_null(o0()));
  ASSERT_FALSE(opaque_is_null(u2o(1)));
  int ref = 0;
  ASSERT_FALSE(opaque_is_null(p2o(&ref)));
}
