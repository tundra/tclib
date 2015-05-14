//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PIPE_H
#define _TCLIB_PIPE_H

#include "c/stdc.h"
#include "sync/sync.h"

// Opaque process type.
typedef struct {
  platform_pipe_t pipe;
} native_pipe_t;

#endif // _TCLIB_PIPE_H
