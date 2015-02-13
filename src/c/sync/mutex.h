//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_MUTEX_H
#define _TCLIB_MUTEX_H

#include "c/stdc.h"
#include "sync/sync.h"

// Opaque mutex type.
typedef struct {
  bool is_initialized;
  platform_mutex_t mutex;
} native_mutex_t;

// Create a new uninitialized mutex.
void native_mutex_construct(native_mutex_t *mutex);

// Dispose the given mutex.
void native_mutex_dispose(native_mutex_t *mutex);

// Initialize the given mutex. Returns true iff initialization succeeds.
bool native_mutex_initialize(native_mutex_t *mutex);

// Lock the given mutex. If it's already held by a different thread we'll wait
// for it to be released. If it's already held by this thread that's fine, we'll
// lock it again.
bool native_mutex_lock(native_mutex_t *mutex);

// Lock the given mutex. If it's already held don't wait but return false
// immediately.
bool native_mutex_try_lock(native_mutex_t *mutex);

// Unlock the given mutex. Only the thread that holds the mutex will be allowed
// to unlock it. If another thread tries to unlock it there are two possible
// behaviors depending on which platform you're on. On some it will be caught
// and unlocking will return false, after which the mutex is still in a valid
// state. On others the behavior and return value will be undefined and the
// mutex may be left broken. If checks_consistency() returns true the first
// will be the case, otherwise it will be the second.
bool native_mutex_unlock(native_mutex_t *mutex);

// Returns true if unlocking while a mutex isn't held causes unlock to
// return false. If this returns false the discipline is implicit and
// unlocking inconsistently may leave the mutex permanently broken. Since it's
// assumed that locks are used consistently in production code this is for
// testing obviously.
bool native_mutex_checks_consistency();

#endif // _TCLIB_MUTEX_H
