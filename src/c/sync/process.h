//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PROCESS_H
#define _TCLIB_PROCESS_H

#include "c/stdc.h"

#include "async/promise.h"
#include "sync/pipe.h"
#include "sync/sync.h"

typedef struct stream_redirector_t stream_redirector_t;

typedef struct native_process_t native_process_t;

// Helper class that controls how an I/O device is used as a standard stream
// connected to a process. Usually it's not just a matter of connecting a
// stream to stdout, say, some extra management needs to be done based on the
// type of the input. For instance, with pipes you need to connect one end and
// close the other.
typedef struct {
  const stream_redirector_t *redirector_;
  opaque_t o_data_;
} stream_redirect_t;

// Identifiers for the standard streams.
typedef enum {
  siStdin = 0,
  siStdout = 1,
  siStderr = 2
} stdio_stream_t;

// Flags that control how a process behaves.
typedef enum {
  // If this flag is set the process will be created in a suspended state and
  // has to be explicitly unsuspended before it will run.
  pfStartSuspendedOnWindows = 0x01,
  // If this flag is set the process will get a new console with the console
  // window hidden.
  pfNewHiddenConsoleOnWindows = 0x02,
  // Similar to pfNewHiddenConsoleOnWindows but displays the console window.
  pfNewVisibleConsoleOnWindows = 0x04
} native_process_flags_t;

// How many standard streams are there?
#define kStdioStreamCount 3

// Create and initialize a new native process.
native_process_t *native_process_new();

// Dispose and free a process created using native_process_new.
void native_process_destroy(native_process_t *process);

// Redirects a standard stream of the given process to the given value.
void native_process_set_stream(native_process_t *process, stdio_stream_t stream,
    stream_redirect_t value);

// Start this process running. This will return immediately after spawning
// the child process, there is no guarantee that the executable is started or
// indeed completes successfully.
bool native_process_start(native_process_t *process, utf8_t executable,
    size_t argc, utf8_t *argv);

// Wait for this process, which must already have been started, to complete.
// Returns true iff waiting succeeded. The process must have been started.
bool native_process_wait(native_process_t *process);

// Returns the process' exit code. The process must have been started and
// waited on.
opaque_promise_t *native_process_exit_code(native_process_t *process);

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
stream_redirect_t stream_redirect_from_pipe(native_pipe_t *pipe,
    pipe_direction_t dir);

#endif // _TCLIB_PROCESS_H
