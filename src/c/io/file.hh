//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_FILE_HH
#define _TCLIB_FILE_HH

#include "stdc.h"

BEGIN_C_INCLUDES
#include "file.h"
END_C_INCLUDES

#include "std/stdvector.hh"

struct file_system_t { };
struct io_stream_t { };

namespace tclib {

// Handle for an open file.
class IoStream : public io_stream_t {
public:
  virtual ~IoStream() { }

  // Attempt to read 'size' bytes from this stream, storing the data at the given
  // destination. Returns the number of bytes actually read.
  virtual size_t read_bytes(void *dest, size_t size) = 0;

  // Attempt to write 'size' bytes to this stream. Returns the number of bytes
  // actually written.
  virtual size_t write_bytes(void *src, size_t size) = 0;

  // Returns true if this file has been read to the end.
  virtual bool at_eof() = 0;

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
  virtual IoStream *open(const char *path, open_file_mode_t mode) = 0;

  // Returns a handle for standard input. The result should be statically
  // allocated so you don't need to dispose it after use.
  virtual IoStream *std_in() = 0;

  // Returns a handle for standard output. The result should be statically
  // allocated so you don't need to dispose it after use.
  virtual IoStream *std_out() = 0;

  // Returns a handle for standard error. The result should be statically
  // allocated so you don't need to dispose it after use.
  virtual IoStream *std_err() = 0;

  virtual ~FileSystem() { }

  // Returns the native file system implementation.
  static FileSystem *native();
};

// An io stream that reads data from a block of bytes and ignores writes.
class ByteInStream : public IoStream {
public:
  ByteInStream(byte_t *data, size_t size);
  virtual size_t read_bytes(void *dest, size_t size);
  virtual size_t write_bytes(void *src, size_t size);
  virtual bool at_eof();
  virtual size_t vprintf(const char *fmt, va_list argp);
  virtual bool flush();

private:
  byte_t *data_;
  size_t size_;
  size_t cursor_;
};

// An io stream that writes data to a vector and ignores reads.
class ByteOutStream : public IoStream {
public:
  ByteOutStream();
  virtual size_t read_bytes(void *dest, size_t size);
  virtual size_t write_bytes(void *src, size_t size);
  virtual bool at_eof();
  virtual size_t vprintf(const char *fmt, va_list argp);
  virtual bool flush();

  // Returns the number of bytes written to this stream.
  size_t size() { return data_.size(); }

  // Returns the data stored so far in this stream.
  std::vector<byte_t> &data() { return data_; }

private:
  std::vector<byte_t> data_;
};

} // tclib

#endif // _TCLIB_FILE_HH
