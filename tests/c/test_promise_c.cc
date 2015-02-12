//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "async/promise.h"
END_C_INCLUDES

#define kValue 62634

static opaque_t count_and_check(opaque_t raw_counter, opaque_t raw_value) {
  ASSERT_EQ(kValue, o2u(raw_value));
  size_t *counter = (size_t*) o2p(raw_counter);
  (*counter)++;
  return opaque_null();
}

TEST(promise_c, simple_success) {
  opaque_promise_t *promise = opaque_promise_empty();
  ASSERT_FALSE(opaque_promise_is_resolved(promise));
  size_t successes = 0;
  size_t failures = 0;
  unary_callback_t *on_success = unary_callback_new_1(count_and_check, p2o(&successes));
  opaque_promise_on_success(promise, on_success);
  ASSERT_EQ(0, successes);
  unary_callback_t *on_failure = unary_callback_new_1(count_and_check, p2o(&failures));
  opaque_promise_on_failure(promise, on_failure);
  ASSERT_EQ(0, failures);
  opaque_promise_fulfill(promise, u2o(kValue));
  ASSERT_EQ(1, successes);
  ASSERT_EQ(0, failures);
  ASSERT_TRUE(opaque_promise_is_resolved(promise));
  opaque_promise_on_success(promise, on_success);
  ASSERT_EQ(2, successes);
  ASSERT_EQ(0, failures);
  opaque_promise_on_failure(promise, on_failure);
  ASSERT_EQ(2, successes);
  ASSERT_EQ(0, failures);
  opaque_promise_destroy(promise);
  callback_destroy(on_success);
  callback_destroy(on_failure);
}

TEST(promise_c, simple_failure) {
  opaque_promise_t *promise = opaque_promise_empty();
  ASSERT_FALSE(opaque_promise_is_resolved(promise));
  size_t successes = 0;
  size_t failures = 0;
  unary_callback_t *on_success = unary_callback_new_1(count_and_check, p2o(&successes));
  opaque_promise_on_success(promise, on_success);
  ASSERT_EQ(0, successes);
  unary_callback_t *on_failure = unary_callback_new_1(count_and_check, p2o(&failures));
  opaque_promise_on_failure(promise, on_failure);
  ASSERT_EQ(0, failures);
  opaque_promise_fail(promise, u2o(kValue));
  ASSERT_EQ(0, successes);
  ASSERT_EQ(1, failures);
  ASSERT_TRUE(opaque_promise_is_resolved(promise));
  opaque_promise_on_success(promise, on_success);
  ASSERT_EQ(0, successes);
  ASSERT_EQ(1, failures);
  opaque_promise_on_failure(promise, on_failure);
  ASSERT_EQ(0, successes);
  ASSERT_EQ(2, failures);
  opaque_promise_destroy(promise);
  callback_destroy(on_success);
  callback_destroy(on_failure);
}
