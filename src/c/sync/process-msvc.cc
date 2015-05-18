//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

BEGIN_C_INCLUDES
#include "utils/strbuf.h"
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
  bool configure_standard_streams();
  bool launch(const char *executable);
  bool post_launch();
private:
  NativeProcess *process_;
  string_buffer_t cmdline_buf_;
  utf8_t cmdline_;
  STARTUPINFO startup_info_;
  handle_t stdout_handle_;
};
}

NativeProcessStart::NativeProcessStart(NativeProcess *process)
  : process_(process)
  , stdout_handle_(AbstractStream::kNullNakedFileHandle) {
  string_buffer_init(&cmdline_buf_);
  ZeroMemory(&startup_info_, sizeof(startup_info_));
  startup_info_.cb = sizeof(startup_info_);
}

NativeProcessStart::~NativeProcessStart() {
  string_buffer_dispose(&cmdline_buf_);
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

bool NativeProcessStart::configure_standard_streams() {
  bool has_set_stream = false;
  if (process_->stdout_ != NULL) {
    stdout_handle_ = process_->stdout_->to_raw_handle();
    if (stdout_handle_ == AbstractStream::kNullNakedFileHandle) {
      WARN("Invalid stdout");
      return false;
    }
    startup_info_.hStdOutput = stdout_handle_;
    has_set_stream = true;
  }
  if (has_set_stream)
    startup_info_.dwFlags |= STARTF_USESTDHANDLES;
  return true;
}

bool NativeProcessStart::launch(const char *executable) {
  char *cmdline_chars = const_cast<char*>(cmdline_.chars);
  // Create the child process.
  bool code = CreateProcess(
    executable,                  // lpApplicationName
    cmdline_chars,               // lpCommandLine
    NULL,                        // lpProcessAttributes
    NULL,                        // lpThreadAttributes
    true,                        // bInheritHandles
    0,                           // dwCreationFlags
    NULL,                        // lpEnvironment
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

bool NativeProcessStart::post_launch() {
  // Close the parent's clone of the stdout handle since it belongs to the
  // child now.
  if (stdout_handle_ != AbstractStream::kNullNakedFileHandle) {
    CloseHandle(stdout_handle_);
    stdout_handle_ = AbstractStream::kNullNakedFileHandle;
  }
  return true;
}

bool NativeProcess::start(const char *executable, size_t argc, const char **argv) {
  CHECK_EQ("starting process already running", nsInitial, state);
  NativeProcessStart start(this);
  start.build_cmdline(executable, argc, argv);
  return start.configure_standard_streams()
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
