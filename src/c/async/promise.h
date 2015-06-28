//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PROMISE_H
#define _TCLIB_PROMISE_H

#include "c/stdc.h"
#include "utils/callback.h"
#include "utils/opaque.h"

// A promise (basically a promise_t) that resolved or fails to an opaque_t. See
// opaque_t for details.
typedef struct opaque_promise_t opaque_promise_t;

typedef enum {
  omTakeOwnership,
  omDontTakeOwnership
} ownership_mode_t;

// Creates and returns a new empty opaque promise.
opaque_promise_t *opaque_promise_empty();

// Returns a new promise whose value is the result of invoking the given
// callback to the value of the given promise. If the promise fails, the result
// also fails. The ownership mode controls ownership of the callback.
opaque_promise_t *opaque_promise_then(opaque_promise_t *promise,
    unary_callback_t *callback, ownership_mode_t ownership);

// Returns true iff the given promise has been resolved, success or failure.
bool opaque_promise_is_resolved(opaque_promise_t *promise);

// Returns the value of the given promise if it has been fulfilled, otherwise
// the given fallback value.
opaque_t opaque_promise_peek_value(opaque_promise_t *promise, opaque_t otherwise);

// Fulfill an opaque promise with the given value, if it hasn't already been
// resolved. Returns whether the promise was resolved.
bool opaque_promise_fulfill(opaque_promise_t *promise, opaque_t value);

// Fail an opaque promise with the given value, if it hasn't already been
// resolved.
bool opaque_promise_fail(opaque_promise_t *promise, opaque_t error);

// Adds a callback to the set that will be invoked when the given promise
// succeeds. The callback must not be disposed until after it has been called;
// it is safe do dispose it during the call itself.
void opaque_promise_on_success(opaque_promise_t *promise, unary_callback_t *callback,
    ownership_mode_t ownership);

// Adds a callback to the set that will be invoked when the given promise
// fails. The callback must not be disposed until after it has been called;
// it is safe do dispose it during the call itself.
void opaque_promise_on_failure(opaque_promise_t *promise, unary_callback_t *callback,
    ownership_mode_t ownership);

// Disposes an opaque promise.
void opaque_promise_destroy(opaque_promise_t *value);

#endif // _TCLIB_PROMISE_H
