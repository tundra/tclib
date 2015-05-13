//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

void NativeProcess::platform_initialize() {
  process = 0;
}

void NativeProcess::platform_dispose() {
  // Nothing to do.
}

bool NativeProcess::start(const char *executable, size_t argc, const char **argv) {
  CHECK_EQ("starting process already running", nsInitial, state);
  pid_t fork_pid = fork();
  if (fork_pid == -1) {
    // Forking failed for some reason.
    WARN("Call to fork failed: %i", fork_pid);
    return false;
  } else if (fork_pid > 0) {
    // We're in the parent so just record the child's pid and we're done.
    this->process = fork_pid;
    this->state = nsRunning;
    return true;
  }
  // We must be in the child process. Run the executable.
  char **args = new char *[argc + 2];
  args[0] = const_cast<char*>(executable);
  for (size_t i = 0; i < argc; i++)
    args[i + 1] = const_cast<char*>(argv[i]);
  args[argc + 1] = NULL;
  execv(executable, args);
  // If successful execv replaces the process so we'll never reach this point.
  // If it fails though we'll drop to here and we need to bail out immediately.
  // The error code signals an os api error.
  exit(EX_OSERR);
  // This should never be reached.
  ERROR("Fell through exec, pid %i", getpid());
  return false;
}

bool NativeProcess::wait() {
  CHECK_EQ("waiting for process not running", nsRunning, state);
  pid_t pid = waitpid(this->process, &this->result, 0);
  if (pid == -1) {
    WARN("Failed to wait for pid %i", this->process);
    return false;
  } else {
    state = nsComplete;
    return true;
  }
}

int NativeProcess::exit_code() {
  CHECK_EQ("getting exit code of running process", nsComplete, state);
  return WEXITSTATUS(result);
}
