//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_ATOMIC_H
#define _TCLIB_ATOMIC_H

#include "c/stdc.h"
#include "sync/sync.h"

// A 32-bit value that can be manipulated atomically. Atomic ints are very
// lightweight to create and maintain through the inc/dec operations may or may
// not be slightly expensive. Clearing the memory of an atomic int32 to zeroes
// is meaningful and causes the value to be cleared to 0.
typedef struct {
  volatile int32_t value;
} atomic_int32_t;

// Increment the given atomic value.
int32_t atomic_int32_increment(atomic_int32_t *value);

// Decrement the given atomic value.
int32_t atomic_int32_decrement(atomic_int32_t *value);

// Atomically add the given delta to the given value.
int32_t atomic_int32_add(atomic_int32_t *value, int32_t delta);

// Atomically subtract the given delta from the given value.
int32_t atomic_int32_subtract(atomic_int32_t *value, int32_t delta);

// Returns the current value of the given atomic integer.
int32_t atomic_int32_get(atomic_int32_t *value);

bool atomic_int32_compare_and_set(atomic_int32_t *value, int32_t old_value,
    int32_t new_value);

int32_t atomic_int32_set(atomic_int32_t *value, int32_t new_value);

// Returns a new atomic int32 that starts out with the given value. The result
// does not have to be disposed.
atomic_int32_t atomic_int32_new(int32_t value);

// A 64-bit value that can be manipulated atomically. Atomic ints are very
// lightweight to create and maintain through the inc/dec operations may or may
// not be slightly expensive. Clearing the memory of an atomic int64 to zeroes
// is meaningful and causes the value to be cleared to 0.
typedef struct {
  volatile int64_t value;
} atomic_int64_t;

// Increment the given atomic value.
int64_t atomic_int64_increment(atomic_int64_t *value);

// Decrement the given atomic value.
int64_t atomic_int64_decrement(atomic_int64_t *value);

// Atomically add the given delta to the given value.
int64_t atomic_int64_add(atomic_int64_t *value, int64_t delta);

// Atomically subtract the given delta from the given value.
int64_t atomic_int64_subtract(atomic_int64_t *value, int64_t delta);

// Returns the current value of the given atomic integer.
int64_t atomic_int64_get(atomic_int64_t *value);

// Returns a new atomic int64 that starts out with the given value. The result
// does not have to be disposed.
atomic_int64_t atomic_int64_new(int64_t value);

#endif // _TCLIB_ATOMIC_H
