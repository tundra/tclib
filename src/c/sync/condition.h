//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_CONDITION_H
#define _TCLIB_CONDITION_H

#include "c/stdc.h"
#include "sync/sync.h"

// Opaque condition variable type.
typedef struct {
  bool is_initialized;
  platform_condition_t cond;
} native_condition_t;

// Create a new uninitialized condition variable.
void native_condition_construct(native_condition_t *cond);

// Dispose the given condition variable.
void native_condition_dispose(native_condition_t *cond);

#endif // _TCLIB_CONDITION_H
