//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_STREAM_HH
#define _TCLIB_STREAM_HH

#include "c/stdc.h"

#include "c/stdvector.hh"
#include "io/stream.hh"

struct in_stream_t { };
struct out_stream_t { };

namespace tclib {

// Behavior shared between in- and out-streams.
class AbstractStream {
public:
  virtual ~AbstractStream() { }

  // Close this stream. It depends on the type of the stream what closing it
  // means but for an output stream it means that no more output will be written
  // and if someone is listening on the other end it may be signaled to them.
  // For in input stream it signals that no further input will be read and if
  // someone is writing to the stream it may be signaled to them. Returns false
  // iff closing failed, that is, true if it succeeds or if no action was taken.
  // The default implementation does nothing and consequently returns true.
  virtual bool close();
};

class InStream : public in_stream_t, public AbstractStream {
public:
  virtual ~InStream() { }

  // Attempt to read 'size' bytes from this stream, storing the data at the
  // given destination. Returns the number of bytes actually read.
  virtual size_t read_bytes(void *dest, size_t size) = 0;

  // Returns true if this stream has been read to the end.
  virtual bool at_eof() = 0;
};

class OutStream : public out_stream_t, public AbstractStream {
public:
  virtual ~OutStream() { }

  // Attempt to write 'size' bytes to this stream. Returns the number of bytes
  // actually written.
  virtual size_t write_bytes(const void *src, size_t size) = 0;

  // Works just like normal printf, it just writes to this file.
  virtual size_t printf(const char *fmt, ...);

  // Works just like vprintf, it just writes to this file.
  virtual size_t vprintf(const char *fmt, va_list argp);

  // Flushes any buffered writes. Returns true if flushing succeeded.
  virtual bool flush() = 0;
};

// An io stream that reads data from a block of bytes and ignores writes.
class ByteInStream : public InStream {
public:
  ByteInStream(const void *data, size_t size);
  virtual size_t read_bytes(void *dest, size_t size);
  virtual bool at_eof();

private:
  const byte_t *data_;
  size_t size_;
  size_t cursor_;
};

// An io stream that writes data to a vector and ignores reads.
class ByteOutStream : public OutStream {
public:
  ByteOutStream();
  virtual size_t write_bytes(const void *src, size_t size);
  virtual bool flush();

  // Returns the number of bytes written to this stream.
  size_t size() { return data_.size(); }

  // Returns the data stored so far in this stream.
  std::vector<byte_t> &data() { return data_; }

private:
  std::vector<byte_t> data_;
};

} // tclib

#endif // _TCLIB_STREAM_HH
