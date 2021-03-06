//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_STREAM_HH
#define _TCLIB_STREAM_HH

#include "c/stdc.h"

#include "c/stdvector.hh"
#include "io/iop.hh"
#include "utils/alloc.hh"

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
class AbstractStream : public DefaultDestructable {
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

  // Returns true if this stream represents a terminal.
  //
  // TODO: The implementation on windows is a bit dodgy so before using this for
  //   anything but testing that should be fixed.
  virtual bool is_a_tty();

  // If this stream has an underlying OS file handle (think file descriptor)
  // this method will return it. If not it returns a value equal to
  // kNullNakedFileHandle; this is the default behavior.
  virtual naked_file_handle_t to_raw_handle();

  // A value that signifies that there is no naked file handle.
  static naked_file_handle_t kNullNakedFileHandle;
};

// An input stream is a source from which blocks of bytes can be read. You don't
// read them directly from the stream though, you use ReadOps.
class InStream : public in_stream_t, public AbstractStream {
public:
  virtual ~InStream() { }

protected:
  friend class Iop;
  friend class ReadIop;

  // Perform the given read synchronously.
  virtual bool read_sync(read_iop_state_t *op) = 0;
};

class OutStream : public out_stream_t, public AbstractStream {
public:
  virtual ~OutStream() { }

  // Works just like normal printf but writes to this stream. Returns the number
  // of bytes written.
  virtual size_t printf(const char *fmt, ...);

  // Works just like vprintf but writes to this stream. Returns the number of
  // bytes written.
  virtual size_t vprintf(const char *fmt, va_list argp);

  // Flushes any buffered writes. Returns true if flushing succeeded.
  virtual bool flush() = 0;

protected:
  friend class Iop;
  friend class WriteIop;

  // Perform the given write synchronously.
  virtual bool write_sync(write_iop_state_t *op) = 0;
};

// A stream that allows both reading and writing. You generally don't want to
// use this type, it exists primarily so there's a way to express calls that
// produce a handle that works both ways because it happens to be the case that
// the underlying abstractions do.
class InOutStream : public InStream, public OutStream {
public:
  // Wraps a stream around a naked file handle.
  static pass_def_ref_t<InOutStream> from_raw_handle(naked_file_handle_t handle);
};

// An io stream that reads data from a block of bytes and ignores writes.
class ByteInStream : public InStream {
public:
  ByteInStream(const void *data, size_t size);
  virtual void default_destroy() { default_delete_concrete(this); }

protected:
  virtual bool read_sync(read_iop_state_t *op);

private:
  const byte_t *data_;
  size_t size_;
  size_t cursor_;
};

// An io stream that writes data to a vector and ignores reads.
class ByteOutStream : public OutStream {
public:
  ByteOutStream();
  virtual void default_destroy() { default_delete_concrete(this); }
  virtual bool flush();
  void putchar(byte_t c);

  // Returns the number of bytes written to this stream.
  size_t size() { return data_.size(); }

  // Returns the data stored so far in this stream.
  std::vector<byte_t> &data() { return data_; }

protected:
  virtual bool write_sync(write_iop_state_t *op);

private:
  std::vector<byte_t> data_;
};

// A byte out stream with extra conveniences for treating the data as an ascii
// string.
class StringOutStream : public ByteOutStream {
public:
  // Write the null terminator and return the contents of the stream as a
  // string.
  utf8_t flush_string();
};

} // tclib

#endif // _TCLIB_STREAM_HH
