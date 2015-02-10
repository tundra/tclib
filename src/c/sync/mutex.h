//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_MUTEX_H
#define _TCLIB_MUTEX_H

#include "c/stdc.h"
#include "sync/sync.h"

// Opaque mutex type.
typedef struct native_mutex_t native_mutex_t;

// Create a new uninitialized mutex.
native_mutex_t *new_native_mutex();

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

// Unlock the given mutex. Only the thread that holds this mutex will be allowed
// to unlock it; if another thread tries the result will be undefined. Or
// actually it will probably be well-defined for each platform (for instance,
// on posix it will succeed, on windows it will fail) but across platforms
// you shouldn't depend on any particular behavior.
bool native_mutex_unlock(native_mutex_t *mutex);

#endif // _TCLIB_MUTEX_H
