//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PIPE_H
#define _TCLIB_PIPE_H

#include "c/stdc.h"
#include "sync/sync.h"
#include "io/stream.h"

// Opaque process type.
typedef struct {
  platform_pipe_t pipe_;
  in_stream_t *in_;
  out_stream_t *out_;
  utf8_t name_;
  bool in_is_out_;
} native_pipe_t;

// Create a new pipe with a read-end and a write-end. Returns true iff
// creation was successful. You close the pipe by closing the two streams;
// this also happens automatically when the pipe is destroyed.
bool native_pipe_open(native_pipe_t *pipe);

// Dispose this pipe.
void native_pipe_dispose(native_pipe_t *pipe);

// Returns the read-end of this pipe.
in_stream_t *native_pipe_in(native_pipe_t *pipe);

// Returns the write-end of this pipe.
out_stream_t *native_pipe_out(native_pipe_t *pipe);

#endif // _TCLIB_PIPE_H
