//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_SYNC_WORKLIST_H
#define _TCLIB_SYNC_WORKLIST_H

#include "c/stdc.h"

#include "sync/mutex.h"
#include "sync/semaphore.h"
#include "utils/boundbuf.h"

// A bounded-size worklist that values can be posted to in a thread safe way,
// blocking if the list is full, and taken from non-blocking. Use worklist_t(EC,
// EW) where 'EC' is the number of entries and 'EW' is the number of opaques
// per entry.
typedef struct {
  native_semaphore_t vacancies;
  native_semaphore_t occupied;
  native_mutex_t guard;
} generic_worklist_t;

// See the non-generic version below.
bool generic_worklist_init(generic_worklist_t *generic,
    generic_bounded_buffer_t *generic_buffer, opaque_t *buffer_data,
    size_t capacity, size_t width);

// See the non-generic version below.
bool generic_worklist_schedule(generic_worklist_t *generic,
    generic_bounded_buffer_t *generic_buffer, opaque_t *buffer_data,
    opaque_t *values, size_t elmw, duration_t timeout);

// See the non-generic version below.
bool generic_worklist_take(generic_worklist_t *generic,
    generic_bounded_buffer_t *generic_buffer, opaque_t *buffer_data,
    opaque_t *values_out, size_t elmw, duration_t timeout);

// See the non-generic version below.
void generic_worklist_dispose(generic_worklist_t *generic);

#define __WLNAME__(EC, EW, NAME) JOIN5(worklist, EC, by, EW, NAME)

// Generic bounded buffer type.
#define worklist_t(EC, EW) __WLNAME__(EC, EW, t)

// Expands to the the declaration of a bounded buffer of the given size.
#define DECLARE_WORKLIST(EC, EW)                                               \
typedef struct {                                                               \
  generic_worklist_t generic;                                                  \
  bounded_buffer_t(EC, EW) buffer;                                             \
} worklist_t(EC, EW);                                                          \
bool __WLNAME__(EC, EW, init)(worklist_t(EC, EW)*);                            \
void __WLNAME__(EC, EW, dispose)(worklist_t(EC, EW)*);                         \
bool __WLNAME__(EC, EW, is_empty)(worklist_t(EC, EW)*);                        \
bool __WLNAME__(EC, EW, schedule)(worklist_t(EC, EW)*, opaque_t*, size_t,      \
    duration_t);                                                               \
bool __WLNAME__(EC, EW, take)(worklist_t(EC, EW)*, opaque_t*, size_t,          \
    duration_t)

// Expands to the implementation of a bounded buffer of the given size.
#define IMPLEMENT_WORKLIST(EC, EW)                                             \
bool __WLNAME__(EC, EW, init)(worklist_t(EC, EW) *wl) {                        \
  return generic_worklist_init(&wl->generic, &wl->buffer.generic,              \
      wl->buffer.data, (EC), (EW));                                            \
}                                                                              \
void __WLNAME__(EC, EW, dispose)(worklist_t(EC, EW) *wl) {                     \
  generic_worklist_dispose(&wl->generic);                                      \
}                                                                              \
bool __WLNAME__(EC, EW, is_empty)(worklist_t(EC, EW) *wl) {                    \
  return bounded_buffer_is_empty(EC, EW)(&wl->buffer);                         \
}                                                                              \
bool __WLNAME__(EC, EW, schedule)(worklist_t(EC, EW) *wl, opaque_t *values,    \
    size_t elmw, duration_t timeout) {                                         \
  return generic_worklist_schedule(&wl->generic, &wl->buffer.generic,          \
      wl->buffer.data, values, elmw, timeout);                                 \
}                                                                              \
bool __WLNAME__(EC, EW, take)(worklist_t(EC, EW) *wl, opaque_t *values_out,    \
    size_t elmw, duration_t timeout) {                                         \
  return generic_worklist_take(&wl->generic, &wl->buffer.generic,              \
      wl->buffer.data, values_out, elmw, timeout);                             \
}

// Initializes the given worklist that holds 'EC' elements where each element
// consists of 'EW' opaques.
#define worklist_init(EC, EW) __WLNAME__(EC, EW, init)

// Releases the resources held by the given worklist.
#define worklist_dispose(EC, EW) __WLNAME__(EC, EW, dispose)

// Tries to schedule the give value. Waits for the given amount of time before
// giving up. Returns true if scheduling succeeded, false if it failed either
// because time ran out of there was a system error.
#define worklist_schedule(EC, EW) __WLNAME__(EC, EW, schedule)

// Attempts to take an element from the worklist, returning true if one was
// available to take otherwise false. Never blocks.
#define worklist_take(EC, EW) __WLNAME__(EC, EW, take)

// Are there any elements in the given worklist at this instant?
#define worklist_is_empty(EC, EW) __WLNAME__(EC, EW, is_empty)

DECLARE_WORKLIST(16, 1);
DECLARE_WORKLIST(256, 1);

#endif // _TCLIB_SYNC_WORKLIST_H
