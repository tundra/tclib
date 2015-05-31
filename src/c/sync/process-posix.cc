//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

// This is redundant but it helps IDEs understand what is going on.
#include "process.hh"
using namespace tclib;

// By convention this is available even on windows. See man environ.
extern char **environ;

namespace tclib {
class NativeProcessStart {
public:
  NativeProcessStart(NativeProcess *process);
  bool configure_file_descriptors();
  bool build_sub_environment();
  bool parent_post_fork();
  bool child_post_fork(const char *executable, size_t argc, const char **argv);
private:
  NativeProcess *process_;
  std::vector<char*> new_environ_;
};
}

NativeProcessStart::NativeProcessStart(NativeProcess *process)
  : process_(process) { }

bool NativeProcessStart::configure_file_descriptors() {
  if (process_->stdout_ != NULL) {
    if (!process_->stdout_->prepare_launch())
      return false;
    if (process_->stdout_->remote_handle() == AbstractStream::kNullNakedFileHandle) {
      WARN("Invalid stdout");
      return false;
    }
  }
  if (process_->stderr_ != NULL) {
    if (!process_->stderr_->prepare_launch())
      return false;
    if (process_->stderr_->remote_handle() == AbstractStream::kNullNakedFileHandle) {
      WARN("Invalid stderr");
      return false;
    }
  }
  return true;
}

bool NativeProcessStart::build_sub_environment() {
  if (process_->env_.empty())
    // Fast case if the sub-environment is identical to the caller's. In that
    // case we just leave it NULL and the child won't use it.
    return true;

  // Push the bindings in reverse order to given them priority.
  for (size_t i = process_->env_.size(); i > 0; i--) {
    const char *binding = process_->env_[i - 1].c_str();
    new_environ_.push_back(const_cast<char*>(binding));
  }

  // Then copy the current environment.
  for (char **entry = environ; *entry != NULL; entry++)
    new_environ_.push_back(*entry);

  // Remember to null terminate.
  new_environ_.push_back(NULL);

  return true;
}

static bool close_parent_if_necessary(StreamRedirect *stream) {
  return (stream == NULL) || stream->parent_side_close();
}

bool NativeProcessStart::parent_post_fork() {
  // Any nontrivial streams should be closed because they now belong to the
  // child.
  return close_parent_if_necessary(process_->stdin_)
      && close_parent_if_necessary(process_->stdout_)
      && close_parent_if_necessary(process_->stderr_);
}

static void remap_std_stream(StreamRedirect *redirect, int old_fd) {
  if (redirect == NULL)
    return;
  int new_fd = redirect->remote_handle();
  CHECK_TRUE("invalid fd", new_fd != AbstractStream::kNullNakedFileHandle);
  if (new_fd == old_fd)
    return;
  dup2(new_fd, old_fd);
  close(new_fd);
}

static bool close_child_if_necessary(StreamRedirect *stream) {
  return (stream == NULL) || stream->child_side_close();
}

bool NativeProcessStart::child_post_fork(const char *executable, size_t argc,
    const char **argv) {
  // From here on we're in the child process. First redirect std streams if
  // necessary.
  remap_std_stream(process_->stdin_, STDIN_FILENO);
  remap_std_stream(process_->stdout_, STDOUT_FILENO);
  remap_std_stream(process_->stderr_, STDERR_FILENO);

  if (!close_child_if_necessary(process_->stdin_)
      || !close_child_if_necessary(process_->stdout_)
      || !close_child_if_necessary(process_->stderr_))
    return false;

  // Execv implicitly takes its environment from environ. The underlying data
  // would be disposed when the NativeProcessStart is but we'll never get to
  // that point and execv will copy the environment along the the arguments.
  // See https://www.gnu.org/software/libc/manual/html_node/Executing-a-File.html.
  if (!new_environ_.empty())
    environ = new_environ_.data();

  // Run the executable.
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

bool PipeRedirect::prepare_launch() {
  return true;
}

bool PipeRedirect::parent_side_close() {
  // The child now has a clone of the remote handle so we can close this side.
  return remote_side()->close();
}

bool PipeRedirect::child_side_close() {
  // We have a clone of the local handle which is really only useful to the
  // parent so we close it.
  return local_side()->close();
}

void NativeProcess::platform_initialize() {
  process = 0;
}

void NativeProcess::platform_dispose() {
  // Nothing to do.
}

bool NativeProcess::start(const char *executable, size_t argc, const char **argv) {
  CHECK_EQ("starting process already running", nsInitial, state);

  NativeProcessStart start(this);

  // Do the pre-flight work before doing the actual forking so we can report any
  // errors back in the parent process.
  if (!start.configure_file_descriptors() || !start.build_sub_environment())
    return false;

  // Fork the child.
  pid_t fork_pid = fork();
  if (fork_pid == -1) {
    // Forking failed for some reason.
    WARN("Call to fork failed: %i", fork_pid);
    return false;
  } else if (fork_pid > 0) {
    // We're in the parent so just record the child's pid and we're done.
    this->process = fork_pid;
    this->state = nsRunning;
    return start.parent_post_fork();
  } else {
    return start.child_post_fork(executable, argc, argv);
  }
}

bool NativeProcess::wait() {
  CHECK_EQ("waiting for process not running", nsRunning, state);
  pid_t pid = waitpid(this->process, &this->result, 0);
  if (pid == -1 && (errno != ECHILD)) {
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
