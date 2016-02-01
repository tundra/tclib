//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_FILE_HH
#define _TCLIB_FILE_HH

#include "c/stdc.h"
#include "io/stream.hh"

BEGIN_C_INCLUDES
#include "io/file.h"
END_C_INCLUDES

struct file_system_t { };

namespace tclib {

class FileHandle;

// A collection of file handles that gives access to a file.
class FileStreams : public file_streams_t {
public:
  FileStreams();
  FileStreams(FileHandle *file, InStream *in, OutStream *out);

  // Returns the file input stream, or NULL if the file was not opened in read
  // mode.
  InStream *in();

  // Returns the file output stream, or NULL if the file was not opened in write
  // mode.
  OutStream *out();

  // Closes this file.
  void close();
};

// Abstraction that represents a file system. A file system may represent
// the OS's file system but not necessarily.
class FileSystem : public file_system_t {
public:
  // Attempts to open the file with the given name. If opening succeeds returns
  // a set of open file streams.
  virtual FileStreams open(utf8_t path, int32_t mode) = 0;

  // Returns a handle for standard input. The result should be statically
  // allocated so you don't need to dispose it after use.
  virtual InStream *std_in() = 0;

  // Returns a handle for standard output. The result should be statically
  // allocated so you don't need to dispose it after use.
  virtual OutStream *std_out() = 0;

  // Returns a handle for standard error. The result should be statically
  // allocated so you don't need to dispose it after use.
  virtual OutStream *std_err() = 0;

  virtual ~FileSystem() { }

  // Creates a temporary file name somewhere in the file system. Returns a
  // string value backed by the given buffer. The size must be large enough to
  // hold the platform's preferred path length, typically 1024 is sufficient.
  // No file is created with the returned name. Note that there is no guarantee
  // that another program doesn't generate the same file name if this is called
  // from multiple places.
  static utf8_t get_temporary_file_name(utf8_t unique, char *dest, size_t dest_size);

  // Returns the native file system implementation.
  static FileSystem *native();
};

} // tclib

#endif // _TCLIB_FILE_HH
