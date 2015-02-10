//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_THREAD_H
#define _TCLIB_THREAD_H

#include "sync/sync.h"
#include "utils/callback.h"

// Opaque thread type.
typedef struct native_thread_t native_thread_t;

// Creates and returns a new native thread that will run the given callback
// when started.
native_thread_t *new_native_thread(nullary_callback_t *callback);

// Disposes the given native thread.
void dispose_native_thread(native_thread_t *thread);

// Starts the given thread running.
bool native_thread_start(native_thread_t *thread);

// Waits for the given thread to finish, then returns the result returned from
// the thread's callback. Or, technically what gets returned is the pointer
// value of the returned opaque, you can't reliably return anything wider than
// a pointer.
void *native_thread_join(native_thread_t *thread);

// Returns the id of the calling thread.
native_thread_id_t native_thread_get_current_id();

// Returns true iff the two thread ids correspond to the same thread.
bool native_thread_ids_equal(native_thread_id_t a, native_thread_id_t b);

#endif // _TCLIB_THREAD_H
