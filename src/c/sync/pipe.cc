//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "sync/pipe.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
#include "sync/pipe.h"
END_C_INCLUDES

using namespace tclib;

NativePipe::NativePipe() {
  in_ = NULL;
  out_ = NULL;
}

bool native_pipe_open(native_pipe_t *raw_pipe) {
  NativePipe *pipe = static_cast<NativePipe*>(raw_pipe);
  new (pipe) NativePipe();
  return pipe->open(NativePipe::pfDefault);
}

void native_pipe_dispose(native_pipe_t *pipe) {
  static_cast<NativePipe*>(pipe)->~NativePipe();
}

in_stream_t *native_pipe_in(native_pipe_t *pipe) {
  return pipe->in_;
}

out_stream_t *native_pipe_out(native_pipe_t *pipe) {
  return pipe->out_;
}

NativePipe::~NativePipe() {
  delete in();
  in_ = NULL;
  delete out();
  out_ = NULL;
}

const PipeRedirector NativePipe::kInRedir(pdIn);
const PipeRedirector NativePipe::kOutRedir(pdOut);

StreamRedirect NativePipe::redirect(pipe_direction_t dir) {
  const PipeRedirector *redir = (dir == pdIn) ? &kInRedir : &kOutRedir;
  return StreamRedirect(redir, this);
}

#ifdef IS_GCC
#  include "pipe-posix.cc"
#endif

#ifdef IS_MSVC
#  include "pipe-msvc.cc"
#endif
