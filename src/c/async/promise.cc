//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "async/promise.h"
END_C_INCLUDES

#include "async/promise-inl.hh"

using namespace tclib;

typedef promise_t<opaque_t, opaque_t> naked_opaque_promise_t;

static opaque_promise_t *clone_and_cloak(naked_opaque_promise_t promise) {
  naked_opaque_promise_t *result = new naked_opaque_promise_t(promise);
  return reinterpret_cast<opaque_promise_t*>(result);
}

// This is a little subtle but by returning the result by value rather than a
// pointer or a reference the promise will be reffed for the duration of
// whatever call it will be used in which means that we're safe against the
// call indirectly causing the promise to be disposed -- it won't be because
// of that extra ref, and only when the call returns will that count be dereffed
// and the promise actually deleted.
static naked_opaque_promise_t uncloak(opaque_promise_t *promise) {
  return *reinterpret_cast<naked_opaque_promise_t*>(promise);
}

opaque_promise_t *opaque_promise_empty() {
  return clone_and_cloak(naked_opaque_promise_t::empty());
}

bool opaque_promise_is_resolved(opaque_promise_t *promise) {
  return uncloak(promise).is_resolved();
}

bool opaque_promise_fulfill(opaque_promise_t *promise, opaque_t value) {
  return uncloak(promise).fulfill(value);
}

bool opaque_promise_fail(opaque_promise_t *promise, opaque_t error) {
  return uncloak(promise).fail(error);
}

static void opaque_promise_callback_trampoline(unary_callback_t *callback,
    opaque_t value) {
  unary_callback_call(callback, value);
}

void opaque_promise_on_success(opaque_promise_t *promise, unary_callback_t *callback) {
  uncloak(promise).on_success(new_callback(opaque_promise_callback_trampoline,
      callback));
}

void opaque_promise_on_failure(opaque_promise_t *promise, unary_callback_t *callback) {
  uncloak(promise).on_failure(new_callback(opaque_promise_callback_trampoline,
      callback));
}

void opaque_promise_dispose(opaque_promise_t *raw_promise) {
  delete reinterpret_cast<promise_t<opaque_t>*>(raw_promise);
}
