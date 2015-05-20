//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

BEGIN_C_INCLUDES
#include "utils/string-inl.h"
END_C_INCLUDES

#include "c/winhdr.h"

using namespace tclib;

void NativeProcess::platform_initialize() {
  PROCESS_INFORMATION *info = get_platform_process(this);
  ZeroMemory(info, sizeof(*info));
  info->hProcess = INVALID_HANDLE_VALUE;
  info->hThread = INVALID_HANDLE_VALUE;
}

void NativeProcess::platform_dispose() {
  PROCESS_INFORMATION *info = get_platform_process(this);
  if (info->hProcess != INVALID_HANDLE_VALUE)
    CloseHandle(info->hProcess);
  if (info->hThread != INVALID_HANDLE_VALUE)
    CloseHandle(info->hThread);
  // This is not to actually reinitialize the process, just to clear the process
  // info.
  platform_initialize();
}

namespace tclib {
class NativeProcessStart {
public:
  NativeProcessStart(NativeProcess *process);
  ~NativeProcessStart();
  utf8_t build_cmdline(const char *executable, size_t argc, const char **argv);

  // Set up any standard stream redirection in the startup_info.
  bool configure_standard_streams();

  // If necessary set up redirection using the given stream, storing the result
  // in the three out parameters. This is a little messy but we need to do this
  // a couple of times so it seems worth it.
  bool maybe_redirect_standard_stream(const char *name, StreamRedirect *stream,
      handle_t *handle_out, bool *has_redirected);
  bool configure_sub_environment();
  bool launch(const char *executable);
  bool post_launch();
  bool maybe_close_standard_stream(StreamRedirect *stream);
private:
  NativeProcess *process_;
  string_buffer_t cmdline_buf_;
  utf8_t cmdline_;
  STARTUPINFO startup_info_;
  string_buffer_t new_env_buf_;
  utf8_t new_env_;
};
}

NativeProcessStart::NativeProcessStart(NativeProcess *process)
  : process_(process)
  , new_env_(string_empty()) {
  string_buffer_init(&cmdline_buf_);
  ZeroMemory(&startup_info_, sizeof(startup_info_));
  startup_info_.cb = sizeof(startup_info_);
  string_buffer_init(&new_env_buf_);
}

NativeProcessStart::~NativeProcessStart() {
  string_buffer_dispose(&cmdline_buf_);
  string_buffer_dispose(&new_env_buf_);
}

static void string_buffer_append_escaped(string_buffer_t *buf, const char *str) {
  // Escaping commands on windows is tricky. Quoting strings takes care of most
  // luckily but quotes themselves must be escaped with \s. You'd think you'd
  // then need to also escape \s themselves but if you try that fails -- two
  // slashes in a row is read as two actual slashes, not one escaped one, except
  // for immediately before an escaped quote and then at the end of the string
  // because we'll add an unescaped quote there.
  string_buffer_putc(buf, '"');
  size_t length = strlen(str);
  for (size_t ic = 0; ic < length; ic++) {
    // Count slashes from this point. Usually it will be 0.
    size_t slash_count = 0;
    while (str[ic] == '\\' && ic < length) {
      ic++;
      slash_count++;
    }
    // At this point if we started at a sequence of slashes we will now be
    // immediately past it, possibly just past the end of the string.
    if (ic == length || str[ic] == '"') {
      // If we're at the end or before a quote slashes need to be escaped.
      for (size_t is = 0; is < slash_count; is++) {
        string_buffer_putc(buf, '\\');
        string_buffer_putc(buf, '\\');
      }
      // If we're at the end there's nothing to do, the quote will be added
      // below, otherwise escape the quote.
      if (str[ic] == '"') {
        string_buffer_putc(buf, '\\');
        string_buffer_putc(buf, '"');
      }
    } else {
      // The slashes were followed by a non-special character so we can just
      // output them, no need to escape.
      for (size_t is = 0; is < slash_count; is++)
        string_buffer_putc(buf, '\\');
      string_buffer_putc(buf, str[ic]);
    }
  }
  string_buffer_putc(buf, '"');
}

utf8_t NativeProcessStart::build_cmdline(const char *executable, size_t argc,
    const char **argv) {
  string_buffer_append_escaped(&cmdline_buf_, executable);
  for (size_t i = 0; i < argc; i++) {
    string_buffer_append(&cmdline_buf_, new_c_string(" "));
    string_buffer_append_escaped(&cmdline_buf_, argv[i]);
  }
  return cmdline_ = string_buffer_flush(&cmdline_buf_);
}

bool NativeProcessStart::maybe_redirect_standard_stream(const char *name,
    StreamRedirect *stream, handle_t *handle_out, bool *has_redirected) {
  if (stream == NULL)
    return true;
  if (!stream->prepare_launch())
    return false;
  handle_t handle = stream->remote_handle();
  if (handle == AbstractStream::kNullNakedFileHandle) {
    WARN("Invalid %s", name);
    return false;
  }
  *handle_out = handle;
  *has_redirected = true;
  return true;
}

bool NativeProcessStart::configure_standard_streams() {
  bool has_redirected = false;

  if (!maybe_redirect_standard_stream("stdout", process_->stdout_,
      &startup_info_.hStdOutput, &has_redirected))
    return false;

  if (!maybe_redirect_standard_stream("stderr", process_->stderr_,
      &startup_info_.hStdError, &has_redirected))
    return false;

  if (has_redirected)
    startup_info_.dwFlags |= STARTF_USESTDHANDLES;

  return true;
}

bool NativeProcessStart::configure_sub_environment() {
  if (process_->env_.empty())
    // If we don't want the environment to change we just leave new_env_ empty
    // and launch will do the right thing.
    return true;

  // Copy the new variables also.
  for (size_t i = process_->env_.size(); i > 0; i--) {
    std::string entry = process_->env_[i - 1];
    string_buffer_append(&new_env_buf_, new_string(entry.c_str(), entry.length()));
    string_buffer_putc(&new_env_buf_, '\0');
  }

  // Copy the existing environment into the new env block.
  for (char **entry = environ; *entry != NULL; entry++) {
    string_buffer_append(&new_env_buf_, new_c_string(*entry));
    string_buffer_putc(&new_env_buf_, '\0');
  }

  // Flushing adds the last of the two null terminators that ends the whole
  // thing.
  new_env_ = string_buffer_flush(&new_env_buf_);
  return true;
}

bool NativeProcessStart::launch(const char *executable) {
  char *cmdline_chars = const_cast<char*>(cmdline_.chars);
  void *env = NULL;
  if (!string_is_empty(new_env_))
    env = static_cast<void*>(const_cast<char*>(new_env_.chars));

  // Create the child process.
  bool code = CreateProcess(
    executable,                  // lpApplicationName
    cmdline_chars,               // lpCommandLine
    NULL,                        // lpProcessAttributes
    NULL,                        // lpThreadAttributes
    true,                        // bInheritHandles
    0,                           // dwCreationFlags
    env,                         // lpEnvironment
    NULL,                        // lpCurrentDirectory
    &startup_info_,              // lpStartupInfo
    get_platform_process(process_)); // lpProcessInformation

  if (code) {
    process_->state = nsRunning;
  } else {
    process_->state = nsCouldntCreate;
    process_->result = GetLastError();
  }

  // It might seem counter-intuitive to always succeed, even if CreateProcess
  // returns false, but it is done to keep the interface consistent across
  // platforms. On posix we won't know if creating the child failed until after
  // we wait for it so even in that case start will succeed. So we simulate that
  // on windows by succeeding here and recording the error so it can be reported
  // later.
  return true;
}

bool NativeProcessStart::maybe_close_standard_stream(StreamRedirect *stream) {
  return (stream == NULL) || stream->parent_side_close();
}

bool NativeProcessStart::post_launch() {
  // Close the parent's clone of the stdout handle since it belongs to the
  // child now.
  return maybe_close_standard_stream(process_->stdout_)
      && maybe_close_standard_stream(process_->stdout_);
}

bool PipeRedirect::prepare_launch() {
  // Do inherit the remote side of this pipe.
  if (!SetHandleInformation(
          remote_side()->to_raw_handle(), // hObject
          HANDLE_FLAG_INHERIT,            // dwMask
          1)) {                           // dwFlags
    WARN("Failed to set remote pipe flags while redirecting");
    return false;
  }

  // Don't inherit the local side of this pipe.
  if (!SetHandleInformation(
          local_side()->to_raw_handle(), // hObject
          HANDLE_FLAG_INHERIT,           // dwMask
          0)) {                          // dwFlags
    WARN("Failed to set local pipe flags while redirecting");
    return false;
  }
  return true;
}

bool PipeRedirect::parent_side_close() {
  return remote_side()->close();
}

bool PipeRedirect::child_side_close() {
  // There is no child side, or -- there is but we don't have access to it.
  return true;
}

bool NativeProcess::start(const char *executable, size_t argc, const char **argv) {
  CHECK_EQ("starting process already running", nsInitial, state);
  NativeProcessStart start(this);
  start.build_cmdline(executable, argc, argv);
  return start.configure_standard_streams()
      && start.configure_sub_environment()
      && start.launch(executable)
      && start.post_launch();
}

bool NativeProcess::wait() {
  if (state == nsCouldntCreate) {
    // If we didn't even manage to create the child process waiting for it to
    // terminate trivially succeeds.
    state = nsComplete;
    return true;
  }

  // First, wait for the process to terminate.
  CHECK_EQ("waiting for process not running", nsRunning, state);
  PROCESS_INFORMATION *info = get_platform_process(this);
  dword_t wait_result = WaitForSingleObject(info->hProcess, INFINITE);
  if (wait_result == WAIT_FAILED) {
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
    return false;
  }

  // Then grab the exit code.
  dword_t exit_code = 0;
  if (!GetExitCodeProcess(info->hProcess, &exit_code)) {
    WARN("Call to GetExitCodeProcess failed: %i", GetLastError());
    return false;
  }

  // Only if both steps succeed do we count it as completed.
  state = nsComplete;
  result = exit_code;
  return true;
}

int NativeProcess::exit_code() {
  CHECK_EQ("getting exit code of running process", nsComplete, state);
  return result;
}
