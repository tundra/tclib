//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_STREAM_HH
#define _TCLIB_STREAM_HH

#include "c/stdc.h"

#include "c/stdvector.hh"
#include "io/iop.hh"

BEGIN_C_INCLUDES
#include "io/stream.h"
#include "sync/sync.h"
#include "utils/duration.h"
END_C_INCLUDES

struct in_stream_t { };
struct out_stream_t { };

namespace tclib {

// The type of a naked underlying file handle. We sometimes need to be able to
// get these for a stream in order to interact with OS apis, for instance when
// redirecting the standard streams to a child process.
typedef IF_MSVC(handle_t, int) naked_file_handle_t;

class InStream;
class OutStream;
class Iop;

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

  // If this stream has an underlying OS file handle (think file descriptor)
  // this method will return it. If not it returns a value equal to
  // kNullNakedFileHandle; this is the default behavior.
  virtual naked_file_handle_t to_raw_handle();

  // A value that signifies that there is no naked file handle.
  static naked_file_handle_t kNullNakedFileHandle;
};

class InStream : public in_stream_t, public AbstractStream {
public:
  virtual ~InStream() { }

  // Attempt to read 'dest_size' bytes from this stream, storing the data at the
  // given destination. If successful stores he number of bytes actually read
  // in the out parameter and returns true, otherwise returns false.
  virtual bool read_bytes(void *dest, size_t dest_size, size_t *read_out);

protected:
  friend class Iop;
  friend class ReadIop;

  // Perform the given iop synchronously.
  virtual bool read_sync(read_iop_t *op) = 0;
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
  virtual bool read_sync(read_iop_t *op);
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
