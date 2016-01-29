//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <unistd.h>
#include <errno.h>

BEGIN_C_INCLUDES
#include "utils/string-inl.h"
END_C_INCLUDES

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

class PosixServerPipe : public ServerPipe {
public:
  PosixServerPipe();
  virtual ~PosixServerPipe();

  virtual bool open(uint32_t flags);

  virtual utf8_t name() { return string_empty(); }

  virtual InStream *in() { return incoming_.in(); }

  virtual OutStream *out() { return outgoing_.out(); }

  static ServerPipe *create();

private:
  NativePipe incoming_;
  NativePipe outgoing_;
};

PosixServerPipe::PosixServerPipe() { }

PosixServerPipe::~PosixServerPipe() {

}

bool PosixServerPipe::open(uint32_t flags) {
  return incoming_.open(flags) && outgoing_.open(flags);
}

ServerPipe *ServerPipe::create() {
  return new PosixServerPipe();
}
