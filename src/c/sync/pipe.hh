//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PIPE_HH
#define _TCLIB_PIPE_HH

#include "c/stdc.h"

#include "io/stream.hh"

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
};

} // namespace tclib

#endif // _TCLIB_PIPE_HH
