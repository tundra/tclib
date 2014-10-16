//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_FILE_HH
#define _TCLIB_FILE_HH

#include "stdc.h"

BEGIN_C_INCLUDES
#include "file.h"
END_C_INCLUDES

struct file_system_t { };
struct open_file_t { };

namespace tclib {

// Handle for an open file.
class OpenFile : public open_file_t {
public:
  virtual ~OpenFile() { }

  // Attempt to read 'size' bytes from this file, storing the data at the given
  // destination. Returns the number of bytes actually read.
  virtual size_t read_bytes(void *dest, size_t size) = 0;

  // Works just like normal printf, it just writes to this file.
  virtual size_t printf(const char *fmt, ...);

  // Works just like vprintf, it just writes to this file.
  virtual size_t vprintf(const char *fmt, va_list argp) = 0;

  // Flushes any buffered writes. Returns true if flushing succeeded.
  virtual bool flush() = 0;
};

// Abstraction that represents a file system. A file system may represent
// the OS's file system but not necessarily.
class FileSystem : public file_system_t {
public:
  // Attempts to open the file with the given name. If opening succeeds returns
  // an open file, if it fails returns NULL.
  virtual OpenFile *open(const char *path, open_file_mode_t mode) = 0;

  // Returns a handle for standard input. The result should be statically
  // allocated so you don't need to dispose it after use.
  virtual OpenFile *std_in() = 0;

  // Returns a handle for standard output. The result should be statically
  // allocated so you don't need to dispose it after use.
  virtual OpenFile *std_out() = 0;

  // Returns a handle for standard error. The result should be statically
  // allocated so you don't need to dispose it after use.
  virtual OpenFile *std_err() = 0;

  virtual ~FileSystem() { }

  // Returns the native file system implementation.
  static FileSystem *native();
};

} // tclib

#endif // _TCLIB_FILE_HH
