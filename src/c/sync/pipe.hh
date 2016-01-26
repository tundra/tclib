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
    pfInherit = 1,
    // The pipe should be bound to a name such that it can be opened by that
    // name.
    pfGenerateName = 2,
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

  // If this pipe was created with a name using pfGenerateName this will return
  // that name. If not it will return the empty string.
  utf8_t name();

  // Returns a redirect wrapper for this pipe going in the specified direction.
  StreamRedirect redirect(pipe_direction_t dir);

  // Given the name of a pipe ensures that it has been completely destroyed.
  // After a pipe has been closed there is no guarantee that it can still be
  // used but it is possible that resources associated with it are still being
  // held so you should call this once you're done using it.
  static bool ensure_destroyed(utf8_t name);

private:
  static const PipeRedirector kInRedir;
  static const PipeRedirector kOutRedir;
};

} // namespace tclib

#endif // _TCLIB_PIPE_HH
