//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PIPE_HH
#define _TCLIB_PIPE_HH

#include "c/stdc.h"

#include "io/stream.hh"
#include "sync/process.hh"

BEGIN_C_INCLUDES
#include "sync/pipe.h"
#include "sync/sync.h"
END_C_INCLUDES

namespace tclib {

// An os-native pipe.
class NativePipe : public native_pipe_t {
public:
  // Flags that control how this pipe is created.
  enum Flags {
    // Default settings.
    pfDefault = 0,
    // The pipe is inherited by child processes.
    pfInherit = 1
  };

  // Create a new uninitialized pipe.
  NativePipe();

  // Dispose this pipe.
  ~NativePipe();

  // Create a new pipe with a read-end and a write-end. Returns true iff
  // creation was successful. You close the pipe by closing the two streams;
  // this also happens automatically when the pipe is destroyed.
  bool open(uint32_t flags);

  // Returns the read-end of this pipe.
  InStream *in() { return static_cast<InStream*>(in_); }

  // Returns the write-end of this pipe.
  OutStream *out() { return static_cast<OutStream*>(out_); }

  // Returns a redirect wrapper for this pipe going in the specified direction.
  StreamRedirect redirect(pipe_direction_t dir);

private:
  static const PipeRedirector kInRedir;
  static const PipeRedirector kOutRedir;
};

// A server channel is one end of a named bi-directional connection. The other
// end is a client channel which can be connected using the channel's name. The
// server controls the channel in the sense that the server creates and disposes
// it, the client can only connect and communicate.
class ServerChannel : public DefaultDestructable {
public:
  virtual ~ServerChannel() { }

  // Create this server channel. A new name will be generated for clients to
  // connect through. This call will not block. After this has been called you
  // can call name() to get the channel's name.
  virtual bool create(uint32_t flags) = 0;

  // Ensure that we can write to and read from the channel. This call may block
  // until the client side has connected.
  virtual bool open() = 0;

  // Close this channel and release any resources associated with it.
  virtual bool close() = 0;

  // Returns this channel's string name.
  virtual utf8_t name() = 0;

  // Returns the input stream that allows the server to read data written by the
  // client. Don't close this directly, instead it is closed automatically when
  // the channel is closed.
  virtual InStream *in() = 0;

  // Returns the output stream that allows the server to write data that can be
  // read by the client. Don't close this directly, instead it is closed
  // automatically when the channel is closed.
  virtual OutStream *out() = 0;

  // Create and return a new server channel. The result is allocated using the
  // default allocator and should be deleted using tclib::default_delete.
  static pass_def_ref_t<ServerChannel> create();
};

// The remote end of a bi-directional client/server connection.
class ClientChannel : public DefaultDestructable {
public:
  virtual ~ClientChannel() { }

  // Open a connection to the server, possibly blocking until the connection has
  // been made.
  virtual bool open(utf8_t name) = 0;

  // Returns the input stream that allows the client to read data that was
  // written by the server. Don't close this directly, instead it is closed
  // automatically when the channel is closed.
  virtual InStream *in() = 0;

  // Returns the output stream that allows the client to write data that can be
  // read by the server. Don't close this directly, instead it is closed
  // automatically when the channel is closed.
  virtual OutStream *out() = 0;

  // Creates and returns a new client channel. The result is allocated using the
  // default allocator and should be deleted using tclib::default_delete.
  static pass_def_ref_t<ClientChannel> create();
};

} // namespace tclib

#endif // _TCLIB_PIPE_HH
