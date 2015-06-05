//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_SYNC_WORKLIST_H
#define _TCLIB_SYNC_WORKLIST_H

#include "c/stdc.h"

#include "sync/mutex.h"
#include "sync/semaphore.h"
#include "utils/boundbuf.h"

typedef struct {
  native_semaphore_t vacancies;
  native_mutex_t guard;
} generic_worklist_t;

// Generic bounded buffer type.
#define worklist_t(N) JOIN3(worklist, N, t)

// Expands to the the declaration of a bounded buffer of the given size.
#define DECLARE_WORKLIST(N)                                                    \
typedef struct {                                                               \
  generic_worklist_t generic;                                                  \
  bounded_buffer_t(N) buffer;                                                  \
} JOIN3(worklist, N, t);

DECLARE_WORKLIST(2)

#endif // _TCLIB_SYNC_WORKLIST_H
