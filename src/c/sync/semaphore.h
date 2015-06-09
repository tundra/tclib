//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_SEMAPHORE_H
#define _TCLIB_SEMAPHORE_H

#include "c/stdc.h"
#include "sync/sync.h"

// Opaque semaphore type.
typedef struct {
  // The count immediately after initialization.
  uint32_t initial_count;
  // Has this semaphore been initialized?
  bool is_initialized;
  // Platform-specific data.
  platform_semaphore_t sema;
} native_semaphore_t;

// Construct a new uninitialized semaphore with an initial count of 1.
void native_semaphore_construct(native_semaphore_t *sema);

// Construct a new uninitialized semaphore with the given initial count.
void native_semaphore_construct_with_count(native_semaphore_t *sema,
    uint32_t initial_count);

// Sets the initial count to the given value. Only has an effect if called
// before the semaphore has been initialized.
void native_semaphore_set_initial_count(native_semaphore_t *sema, uint32_t value);

// Initialize the given semaphore, returning true on success.
bool native_semaphore_initialize(native_semaphore_t *sema);

// Attempt to acquire a permit from the given semaphore, blocking up to the
// given duration if necessary.
bool native_semaphore_acquire(native_semaphore_t *sema, duration_t timeout);

// Attempt to acquire a permit from the given semaphore but will not wait if no
// permits are available. Shorthand for native_semaphore_acquire(sema,
// duration_instant()).
bool native_semaphore_try_acquire(native_semaphore_t *sema);

// Release a permit to the given semaphore.
bool native_semaphore_release(native_semaphore_t *sema);

// Dispose the given semaphore.
void native_semaphore_dispose(native_semaphore_t *sema);

#endif // _TCLIB_SEMAPHORE_H
