//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "async/promise-inl.hh"
#include "sync/pipe.hh"
#include "sync/process.hh"
#include "utils/alloc.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
#include "utils/strbuf.h"
END_C_INCLUDES

using namespace tclib;

NativeProcess::NativeProcess()
  : platform_data_(NULL)
  , exit_code_(sync_promise_t<int>::empty())
  , opaque_exit_code_(NULL)
  , stdin_(NULL)
  , stdout_(NULL)
  , stderr_(NULL) {
  state = nsInitial;
}

bool NativeProcess::set_env(utf8_t key, utf8_t value) {
  string_buffer_t buf;
  string_buffer_init(&buf);
  string_buffer_printf(&buf, "%s=%s", key.chars, value.chars);
  utf8_t raw_binding = string_buffer_flush(&buf);
  std::string binding(raw_binding.chars, raw_binding.size);
  env_.push_back(binding);
  string_buffer_dispose(&buf);
  return true;
}

static opaque_t i2o(int value) {
  return u2o(value);
}

opaque_promise_t *NativeProcess::opaque_exit_code() {
  if (opaque_exit_code_ == NULL) {
    promise_t<opaque_t, opaque_t> cpp_promise = exit_code().then(new_callback(i2o),
        new_callback(p2o));
    opaque_exit_code_ = to_opaque_promise(cpp_promise);
  }
  return opaque_exit_code_;
}

bool NativeProcess::wait_sync(Duration timeout) {
  CHECK_TRUE("waiting for process not running", (state == nsRunning)
      || (state == nsCouldntCreate));

  if (state == nsCouldntCreate) {
    // If we didn't even manage to create the child process waiting for it to
    // terminate trivially succeeds.
    state = nsComplete;
    return true;
  }

  // Once the process terminates the drawbridge will be lowered.
  bool passed = exit_code_.wait(timeout);
  if (passed)
    this->state = nsComplete;
  return passed;
}

PipeRedirect::PipeRedirect(NativePipe *pipe, pipe_direction_t direction)
  : pipe_(pipe)
  , direction_(direction) { }

PipeRedirect::PipeRedirect()
  : pipe_(NULL)
  , direction_(pdOut) { }

void PipeRedirect::set_pipe(NativePipe *pipe, pipe_direction_t direction) {
  pipe_ = pipe;
  direction_ = direction;
}

naked_file_handle_t PipeRedirect::remote_handle() {
  return remote_side()->to_raw_handle();
}

AbstractStream *PipeRedirect::remote_side() {
  return is_output() ? static_cast<AbstractStream*>(pipe_->out()) : pipe_->in();
}

AbstractStream *PipeRedirect::local_side() {
  return is_output() ? static_cast<AbstractStream*>(pipe_->in()) : pipe_->out();
}

native_process_t *native_process_new() {
  return new (kDefaultAlloc) NativeProcess();
}

void native_process_destroy(native_process_t *process) {
  default_delete_concrete(static_cast<NativeProcess*>(process));
}

void native_process_set_stdin(native_process_t *process, stream_redirect_t *value) {
  static_cast<NativeProcess*>(process)->set_stdin(static_cast<StreamRedirect*>(value));
}

void native_process_set_stdout(native_process_t *process, stream_redirect_t *value) {
  static_cast<NativeProcess*>(process)->set_stdout(static_cast<StreamRedirect*>(value));
}

void native_process_set_stderr(native_process_t *process, stream_redirect_t *value) {
  static_cast<NativeProcess*>(process)->set_stderr(static_cast<StreamRedirect*>(value));
}

bool native_process_start(native_process_t *process, utf8_t executable,
    size_t argc, utf8_t *argv) {
  return static_cast<NativeProcess*>(process)->start(executable, argc, argv);
}

opaque_promise_t *native_process_exit_code(native_process_t *process) {
  return static_cast<NativeProcess*>(process)->opaque_exit_code();
}

stream_redirect_t *stream_redirect_from_pipe(native_pipe_t *pipe, pipe_direction_t dir) {
  return new PipeRedirect(static_cast<NativePipe*>(pipe), dir);
}

void stream_redirect_destroy(stream_redirect_t *value) {
  delete static_cast<StreamRedirect*>(value);
}

#ifdef IS_GCC
#include "process-posix.cc"
#endif

#ifdef IS_MSVC
#include "process-msvc.cc"
#endif

NativeProcess::~NativeProcess() {
  // Wait for the process to exit before disposing it.
  if (state != nsInitial)
    exit_code_.wait();

  if (opaque_exit_code_ != NULL) {
    opaque_promise_destroy(opaque_exit_code_);
    opaque_exit_code_ = NULL;
  }

  delete platform_data_;
  platform_data_ = NULL;
}
