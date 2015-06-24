//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _UTILS_LIFETIME_H
#define _UTILS_LIFETIME_H

#include "c/stdc.h"

#include "sync/mutex.h"
#include "utils/callback.h"
#include "utils/vector.h"

// A lifetime is a generic structure that can be used to make explicit the life
// of a process. One part of the program defines when the lifetime starts and
// ends and other parts can register actions to take place at the end of the
// lifetime. This is similar to atexit but uses an explicit endpoint rather
// than let the the system determine when the program is exiting.
typedef struct {
  native_mutex_t guard;
  voidp_vector_t callbacks;
} lifetime_t;

// Initializes the state of the given lifetime.
bool lifetime_begin(lifetime_t *lifetime);

// Ends the given lifetime, calling all atexit callbacks and disposing the
// lifetime's state.
void lifetime_end(lifetime_t *lifetime);

// Begins the default lifetime for this process. There can only be one default
// lifetime at a time so calling this while another is the default is an error.
bool lifetime_begin_default(lifetime_t *lifetime);

// Ends the given lifetime which must be the current one for this process.
void lifetime_end_default(lifetime_t *lifetime);

// Returns the current default lifetime. If there is currently no lifetime
// returns NULL.
lifetime_t *lifetime_get_default();

// Adds the given callback to the set called when the given lifetime ende. The
// lifetime takes ownership of the callback.
void lifetime_atexit(lifetime_t *lifetime, nullary_callback_t *callback);

#endif // _UTILS_LIFETIME_H
