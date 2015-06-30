//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_ATOMIC_H
#define _TCLIB_ATOMIC_H

#include "c/stdc.h"
#include "sync/sync.h"

// A 32-bit value that can be manipulated atomically. Atomic ints are very
// lightweight to create and maintain through the inc/dec operations may or may
// not be slightly expensive.
typedef struct {
  volatile int32_t value;
} atomic_int32_t;

// Increment the given atomic value.
int32_t atomic_int32_increment(atomic_int32_t *value);

// Decrement the given atomic value.
int32_t atomic_int32_decrement(atomic_int32_t *value);

// Returns the current value of the given atomic integer.
int32_t atomic_int32_get(atomic_int32_t *value);

// Returns a new atomic int32 that starts out with the given value. The result
// does not have to be disposed.
atomic_int32_t atomic_int32_new(int32_t value);

#endif // _TCLIB_TCLIB_H
