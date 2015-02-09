//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "utils/callback.h"
END_C_INCLUDES

bool operator==(opaque_t a, opaque_t b) {
  return opaque_same(a, b);
}

static int p0 = 0;
static int p1 = 0;
static int p2 = 0;

static opaque_t f0() {
  return p2o(&p0);
}

static opaque_t f1(opaque_t a0) {
  return a0;
}

TEST(callback_c, simple) {
  voidp_callback_t *pcf0 = voidp_callback_0(f0);
  ASSERT_TRUE(p2o(&p0) == voidp_callback_call(pcf0));
  callback_dispose(pcf0);
  voidp_callback_t *pcf1 = voidp_callback_voidp_1(f1, p2o(&p2));
  ASSERT_TRUE(p2o(&p2) == voidp_callback_call(pcf1));
  callback_dispose(pcf1);

  voidp_callback_voidp_t *pcpf1 = voidp_callback_voidp_0(f1);
  ASSERT_TRUE(p2o(NULL) == voidp_callback_voidp_call(pcpf1, p2o(NULL)));
  ASSERT_TRUE(p2o(&p1) == voidp_callback_voidp_call(pcpf1, p2o(&p1)));
  callback_dispose(pcpf1);
}
