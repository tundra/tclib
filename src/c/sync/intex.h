//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_INTEX_H
#define _TCLIB_INTEX_H

#include "c/stdc.h"
#include "sync/condition.h"
#include "sync/mutex.h"
#include "sync/sync.h"

// Opaque condition variable type.
typedef struct {
  bool is_initialized_;
  native_mutex_t guard_;
  native_condition_t cond_;
  volatile uint64_t value_;
} intex_t;

// Constructs the given intex with the given initial value.
void intex_construct(intex_t *intex, uint64_t init_value);

// Initializes the state of this intex, returning true iff initialization
// succeeded.
bool intex_initialize(intex_t *intex);

// Release any resources held by the given intex.
void intex_dispose(intex_t *intex);

// Wait the given duration for the intex to be equal to the given value then
// locks it. Returns true iff successful.
bool intex_lock_when_equal(intex_t *intex, int64_t value, duration_t timeout);

// Wait the given duration for the intex to be less than the given value then
// locks it. Returns true iff successful.
bool intex_lock_when_less(intex_t *intex, int64_t value, duration_t timeout);

// Wait the given duration for the intex to be greater than the given value then
// locks it. Returns true iff successful.
bool intex_lock_when_greater(intex_t *intex, int64_t value, duration_t timeout);

// Unlocks an intex that is already held. Returns true iff successful.
bool intex_unlock(intex_t *intex);

#endif // _TCLIB_INTEX_H
