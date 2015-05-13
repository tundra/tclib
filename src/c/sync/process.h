//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PROCESS_H
#define _TCLIB_PROCESS_H

#include "c/stdc.h"
#include "sync/sync.h"

typedef enum {
  nsInitial,
  nsRunning,
  nsCouldntCreate,
  nsComplete
} native_process_state_t;

// Opaque process type.
typedef struct {
  native_process_state_t state;
  int result;
  platform_process_t process;
} native_process_t;

#endif // _TCLIB_PROCESS_H
