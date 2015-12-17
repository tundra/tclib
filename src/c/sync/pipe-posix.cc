//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <unistd.h>
#include <errno.h>

using namespace tclib;

bool NativePipe::open(uint32_t flags) {
  errno = 0;
  int result = ::pipe(this->pipe_);
  if (result == 0) {
    in_ = InOutStream::from_raw_handle(this->pipe_[0]);
    out_ = InOutStream::from_raw_handle(this->pipe_[1]);
    return true;
  }
  WARN("Call to pipe failed: %i (error: %s)", errno, strerror(errno));
  return false;
}
