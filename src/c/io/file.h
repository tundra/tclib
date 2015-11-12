//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_FILE_H
#define _TCLIB_FILE_H

#include "stream.h"
#include "utils/string.h"

// Modes you can open files in. The values of the enums indicate whether opening
// in that mode produces an in-stream, an out-stream, or both.
typedef enum {
  // Main modes, all files must be opened under one of these.

  OPEN_FILE_MODE_READ = 0x1,
  OPEN_FILE_MODE_WRITE = 0x2,
  OPEN_FILE_MODE_READ_WRITE = 0x4,

  // Additional flags that can be or'ed together with the modes to give fine-
  // grained control.

  // Force binary mode. Has no effect on posix systems, as far as I can tell,
  // but makes a difference on windows.
  OPEN_FILE_FLAG_BINARY = 0x8
} open_file_mode_t;

// Abstraction that represents a file system. A file system may represent
// the OS's file system but not necessarily.
typedef struct file_system_t file_system_t;

// Handle for an open file. This is for internal bookkeeping, it has no external
// use.
typedef struct file_handle_t file_handle_t;

// A collection of handles returned when opening a file.
typedef struct {
  bool is_open;
  file_handle_t *file;
  in_stream_t *in;
  out_stream_t *out;
} file_streams_t;

// Attempts to open the file with the given name. If opening succeeds returns
// a set of streams that allow you to read/write the file, depending on the
// given mode.
file_streams_t file_system_open(file_system_t *fs, utf8_t path, open_file_mode_t mode);

// Closes a set of io streams appropriately.
void file_streams_close(file_streams_t *streams);

// Returns the native file system.
file_system_t *file_system_native();

// Returns a handle for standard input. The result should be statically
// allocated so you don't need to dispose it after use.
in_stream_t *file_system_stdin(file_system_t *fs);

// Returns a handle for standard output. The result should be statically
// allocated so you don't need to dispose it after use.
out_stream_t *file_system_stdout(file_system_t *fs);

// Returns a handle for standard error. The result should be statically
// allocated so you don't need to dispose it after use.
out_stream_t *file_system_stderr(file_system_t *fs);

#endif // _TCLIB_FILE_H
