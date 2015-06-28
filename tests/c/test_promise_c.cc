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
  return o0();
}

TEST(promise_c, simple_success) {
  opaque_promise_t *promise = opaque_promise_empty();
  ASSERT_FALSE(opaque_promise_is_resolved(promise));
  size_t successes = 0;
  size_t failures = 0;
  unary_callback_t *on_success = unary_callback_new_1(count_and_check, p2o(&successes));
  opaque_promise_on_success(promise, on_success, omDontTakeOwnership);
  ASSERT_EQ(0, successes);
  unary_callback_t *on_failure = unary_callback_new_1(count_and_check, p2o(&failures));
  opaque_promise_on_failure(promise, on_failure, omDontTakeOwnership);
  ASSERT_EQ(0, failures);
  opaque_promise_fulfill(promise, u2o(kValue));
  ASSERT_EQ(1, successes);
  ASSERT_EQ(0, failures);
  ASSERT_TRUE(opaque_promise_is_resolved(promise));
  opaque_promise_on_success(promise, on_success, omDontTakeOwnership);
  ASSERT_EQ(2, successes);
  ASSERT_EQ(0, failures);
  opaque_promise_on_failure(promise, on_failure, omDontTakeOwnership);
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
  opaque_promise_on_success(promise, on_success, omDontTakeOwnership);
  ASSERT_EQ(0, successes);
  unary_callback_t *on_failure = unary_callback_new_1(count_and_check, p2o(&failures));
  opaque_promise_on_failure(promise, on_failure, omDontTakeOwnership);
  ASSERT_EQ(0, failures);
  opaque_promise_fail(promise, u2o(kValue));
  ASSERT_EQ(0, successes);
  ASSERT_EQ(1, failures);
  ASSERT_TRUE(opaque_promise_is_resolved(promise));
  opaque_promise_on_success(promise, on_success, omDontTakeOwnership);
  ASSERT_EQ(0, successes);
  ASSERT_EQ(1, failures);
  opaque_promise_on_failure(promise, on_failure, omDontTakeOwnership);
  ASSERT_EQ(0, successes);
  ASSERT_EQ(2, failures);
  opaque_promise_destroy(promise);
  callback_destroy(on_success);
  callback_destroy(on_failure);
}

static opaque_t plus_one(opaque_t o_n) {
  return u2o(o2u(o_n) + 1);
}

TEST(promise_c, then_ownership) {
  opaque_promise_t *p0 = opaque_promise_empty();
  opaque_promise_t *p1 = opaque_promise_then(p0, unary_callback_new_0(plus_one),
      omTakeOwnership);
  opaque_promise_t *p2 = opaque_promise_then(p1, unary_callback_new_0(plus_one),
      omTakeOwnership);
  opaque_promise_t *p3 = opaque_promise_then(p2, unary_callback_new_0(plus_one),
      omTakeOwnership);
  opaque_promise_t *p4 = opaque_promise_then(p3, unary_callback_new_0(plus_one),
      omTakeOwnership);
  opaque_promise_t *p5 = opaque_promise_then(p4, unary_callback_new_0(plus_one),
      omTakeOwnership);

  opaque_promise_fulfill(p0, u2o(317));
  ASSERT_EQ(317, o2u(opaque_promise_peek_value(p0, o0())));
  ASSERT_EQ(318, o2u(opaque_promise_peek_value(p1, o0())));
  ASSERT_EQ(319, o2u(opaque_promise_peek_value(p2, o0())));
  ASSERT_EQ(320, o2u(opaque_promise_peek_value(p3, o0())));
  ASSERT_EQ(321, o2u(opaque_promise_peek_value(p4, o0())));

  opaque_promise_destroy(p0);
  opaque_promise_destroy(p1);
  opaque_promise_destroy(p2);
  opaque_promise_destroy(p3);
  opaque_promise_destroy(p4);
  opaque_promise_destroy(p5);
}
