//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_FILE_H
#define _TCLIB_FILE_H

#include "stdc.h"
#include <stdarg.h>

typedef enum {
  OPEN_FILE_MODE_READ,
  OPEN_FILE_MODE_WRITE,
  OPEN_FILE_MODE_READ_WRITE
} open_file_mode_t;

// Handle for an open file.
typedef struct io_stream_t io_stream_t;

// Abstraction that represents a file system. A file system may represent
// the OS's file system but not necessarily.
typedef struct file_system_t file_system_t;

// Attempt to read 'size' bytes from this file, storing the data at the given
// destination. Returns the number of bytes actually read.
size_t io_stream_read_bytes(io_stream_t *file, void *dest, size_t size);

// Returns true if this file has been read to the end.
bool io_stream_at_eof(io_stream_t *file);

// Works just like normal printf, it just writes to this file.
size_t io_stream_printf(io_stream_t *file, const char *fmt, ...);

// Works just like vprintf, it just writes to this file.
size_t io_stream_vprintf(io_stream_t *file, const char *fmt, va_list argp);

// Closes the given file and frees the object.
void io_stream_close(io_stream_t *file);

// Flushes any buffered writes to the given file.
bool io_stream_flush(io_stream_t *file);

// Attempts to open the file with the given name. If opening succeeds returns
// an open file, if it fails returns NULL.
io_stream_t *file_system_open(file_system_t *fs, const char *path, open_file_mode_t mode);

// Returns the native file system.
file_system_t *file_system_native();

// Returns a handle for standard input. The result should be statically
// allocated so you don't need to dispose it after use.
io_stream_t *file_system_stdin(file_system_t *fs);

// Returns a handle for standard output. The result should be statically
// allocated so you don't need to dispose it after use.
io_stream_t *file_system_stdout(file_system_t *fs);

// Returns a handle for standard error. The result should be statically
// allocated so you don't need to dispose it after use.
io_stream_t *file_system_stderr(file_system_t *fs);

#endif // _TCLIB_FILE_H
