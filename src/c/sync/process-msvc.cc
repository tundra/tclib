//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

BEGIN_C_INCLUDES
#include "utils/strbuf.h"
#include "utils/string-inl.h"
END_C_INCLUDES

#include "c/winhdr.h"

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

bool NativeProcess::start(const char *executable, size_t argc, const char **argv) {
  CHECK_EQ("starting process already running", nsInitial, state);
  // Joing the arguments array together into a command-line.
  // TODO: handle escaping properly.
  string_buffer_t buf;
  string_buffer_init(&buf);
  string_buffer_append(&buf, new_c_string(executable));
  for (size_t i = 0; i < argc; i++) {
    string_buffer_append(&buf, new_c_string(" "));
    string_buffer_append(&buf, new_c_string(argv[i]));
  }
  utf8_t cmdline = string_buffer_flush(&buf);
  char *cmdline_chars = const_cast<char*>(cmdline.chars);
  STARTUPINFO info;
  ZeroMemory(&info, sizeof(info));
  info.cb = sizeof(info);

  // Create the child process.
  bool code = CreateProcess(
    executable,                  // lpApplicationName
    cmdline_chars,               // lpCommandLine
    NULL,                        // lpProcessAttributes
    NULL,                        // lpThreadAttributes
    false,                       // bInheritHandles
    0,                           // dwCreationFlags
    NULL,                        // lpEnvironment
    NULL,                        // lpCurrentDirectory
    &info,                       // lpStartupInfo
    get_platform_process(this)); // lpProcessInformation
  string_buffer_dispose(&buf);

  // If creating the process failed we record the error code.
  if (code) {
    state = nsRunning;
  } else {
    state = nsCouldntCreate;
    result = GetLastError();
  }

  // It might seem counter-intuitive to always succeed, even if CreateProcess
  // returns false, but it is done to keep the interface consistent across
  // platforms. On posix we won't know if creating the child failed until after
  // we wait for it so even in that case start will succeed. So we simulate that
  // on windows by succeeding here and recording the error so it can be reported
  // later.
  return true;
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
