//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "async/promise-inl.hh"
#include "sync/pipe.hh"
#include "sync/process.hh"
#include "utils/alloc.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
#include "utils/strbuf.h"
#include "utils/string-inl.h"
END_C_INCLUDES

using namespace tclib;

StreamRedirect::StreamRedirect() {
  redirector_ = NULL;
  o_data_ = o0();
}

StreamRedirect::StreamRedirect(stream_redirect_t c_redirect) {
  *static_cast<stream_redirect_t*>(this) = c_redirect;
}

StreamRedirect::StreamRedirect(const StreamRedirector *redirector, void *data) {
  redirector_ = redirector;
  o_data_ = p2o(data);
}

const StreamRedirector *StreamRedirect::redirector() {
  return static_cast<const StreamRedirector*>(redirector_);
}

naked_file_handle_t StreamRedirect::remote_handle() {
  return redirector()->remote_handle(this);
}

fat_bool_t StreamRedirect::prepare_launch() {
  return redirector()->prepare_launch(this);
}

fat_bool_t StreamRedirect::parent_side_close() {
  return redirector()->parent_side_close(this);
}

fat_bool_t StreamRedirect::child_side_close() {
  return redirector()->child_side_close(this);
}

NativeProcess::NativeProcess()
  : platform_data_(NULL)
  , exit_code_(sync_promise_t<int>::pending())
  , opaque_exit_code_(NULL)
  , flags_(0) {
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

fat_bool_t NativeProcess::inject_library(InjectRequest *request) {
  F_TRY(start_inject_library(request));
  F_TRY(complete_inject_library(request));
  return F_TRUE;
}

NativeProcess::InjectRequest::InjectRequest(utf8_t path)
  : path_(path)
  , connector_name_(string_empty())
  , data_in_(blob_empty())
  , data_out_(blob_empty())
  , state_(NULL) { }

void NativeProcess::InjectRequest::set_connector(utf8_t name, blob_t data_in,
    blob_t data_out) {
  connector_name_ = name;
  data_in_ = data_in;
  data_out_ = data_out;
}

PipeRedirector::PipeRedirector(pipe_direction_t direction)
  : direction_(direction) { }

naked_file_handle_t PipeRedirector::remote_handle(StreamRedirect *redirect) const {
  return remote_side(redirect)->to_raw_handle();
}

AbstractStream *PipeRedirector::remote_side(StreamRedirect *redirect) const {
  NativePipe *pipe = this->pipe(redirect);
  return is_output() ? static_cast<AbstractStream*>(pipe->out()) : pipe->in();
}

AbstractStream *PipeRedirector::local_side(StreamRedirect *redirect) const {
  NativePipe *pipe = this->pipe(redirect);
  return is_output() ? static_cast<AbstractStream*>(pipe->in()) : pipe->out();
}

native_process_t *native_process_new() {
  return new (kDefaultAlloc) NativeProcess();
}

void native_process_destroy(native_process_t *process) {
  default_delete_concrete(static_cast<NativeProcess*>(process));
}

void native_process_set_stream(native_process_t *process, stdio_stream_t stream,
    stream_redirect_t value) {
  static_cast<NativeProcess*>(process)->set_stream(stream, value);
}

bool native_process_start(native_process_t *process, utf8_t executable,
    size_t argc, utf8_t *argv) {
  return static_cast<NativeProcess*>(process)->start(executable, argc, argv);
}

opaque_promise_t *native_process_exit_code(native_process_t *process) {
  return static_cast<NativeProcess*>(process)->opaque_exit_code();
}

stream_redirect_t stream_redirect_from_pipe(native_pipe_t *pipe, pipe_direction_t dir) {
  return static_cast<NativePipe*>(pipe)->redirect(dir);
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
