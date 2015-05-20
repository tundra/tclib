//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "sync/process.hh"
#include "sync/pipe.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
#include "utils/strbuf.h"
END_C_INCLUDES

using namespace tclib;

NativeProcess::NativeProcess()
  : stdout_(NULL)
  , stderr_(NULL) {
#if defined(kPlatformProcessInit)
  process = kPlatformProcessInit;
#endif
  state = nsInitial;
  result = -1;
  platform_initialize();
}

NativeProcess::~NativeProcess() {
  if (state != nsInitial)
    platform_dispose();
}

bool NativeProcess::set_env(const char *key, const char *value) {
  string_buffer_t buf;
  string_buffer_init(&buf);
  string_buffer_printf(&buf, "%s=%s", key, value);
  utf8_t raw_binding = string_buffer_flush(&buf);
  std::string binding(raw_binding.chars, raw_binding.size);
  env_.push_back(binding);
  string_buffer_dispose(&buf);
  return true;
}

PipeRedirect::PipeRedirect(NativePipe *pipe, pipe_direction_t direction)
  : pipe_(pipe)
  , direction_(direction) { }

naked_file_handle_t PipeRedirect::remote_handle() {
  return remote_side()->to_raw_handle();
}

AbstractStream *PipeRedirect::remote_side() {
  return is_output() ? static_cast<AbstractStream*>(pipe_->out()) : pipe_->in();
}

AbstractStream *PipeRedirect::local_side() {
  return is_output() ? static_cast<AbstractStream*>(pipe_->in()) : pipe_->out();
}

#ifdef IS_GCC
#include "process-posix.cc"
#endif

#ifdef IS_MSVC
#include "process-msvc.cc"
#endif
