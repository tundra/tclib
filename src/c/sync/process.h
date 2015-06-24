//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PROCESS_H
#define _TCLIB_PROCESS_H

#include "c/stdc.h"
#include "sync/pipe.h"
#include "sync/sync.h"

typedef struct stream_redirect_t stream_redirect_t;

typedef struct native_process_t native_process_t;

// Create and initialize a new native process.
native_process_t *native_process_new();

// Dispose and free a process created using native_process_new.
void native_process_destroy(native_process_t *process);

// Sets the stream to redirect this process' stdin to.
void native_process_set_stdin(native_process_t *process, stream_redirect_t *value);

// Sets the stream to redirect this process' stdout to.
void native_process_set_stdout(native_process_t *process, stream_redirect_t *value);

// Sets the stream to redirect this process' stderr to.
void native_process_set_stderr(native_process_t *process, stream_redirect_t *value);

// Start this process running. This will return immediately after spawning
// the child process, there is no guarantee that the executable is started or
// indeed completes successfully.
bool native_process_start(native_process_t *process, const char *executable,
    size_t argc, const char **argv);

// Wait for this process, which must already have been started, to complete.
// Returns true iff waiting succeeded. The process must have been started.
bool native_process_wait(native_process_t *process);

// Returns the process' exit code. The process must have been started and
// waited on.
int native_process_exit_code(native_process_t *process);

// Direction of a pipe when used to redirect input/output to/from a process. The
// direction is defined as viewed from the process so pdIn means that the pipe
// is used to pipe data into the process -- stdin -- and pdOut means that the
// pipe will receive data from the process -- stdout, stderr.
typedef enum {
  pdIn,
  pdOut
} pipe_direction_t;

// Creates a new stream redirect that redirects using the given pipe in either
// read or write mode depending on the direction.
stream_redirect_t *stream_redirect_from_pipe(native_pipe_t *pipe,
    pipe_direction_t dir);

// Disposes the given value's resources and the value itself.
void stream_redirect_destroy(stream_redirect_t *value);

#endif // _TCLIB_PROCESS_H
