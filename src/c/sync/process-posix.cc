//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <errno.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sysexits.h>
#include <unistd.h>

#include "c/stdhashmap.hh"
#include "sync/intex.hh"
#include "sync/mutex.hh"
#include "sync/semaphore.hh"
#include "sync/thread.hh"

BEGIN_C_INCLUDES
#include "utils/lifetime.h"
END_C_INCLUDES

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

// The process registry is a global mapping from child process pids to process
// objects. It's a somewhat heavy thing but it appears to be the simplest way
// to tie signals and processes together.
//
// The registry works as follows. The first time you create a process the
// registry gets created. It schedules cleanup using the default lifetime so you
// need one of those. When a child signal is received it immediately passes it
// on to a helper thread which it owns. The helper thread resolves which process
// object corresponds to the child and passes the message on to that. The reason
// for dispatching from a helper thread is that unless we want to severely
// restrict actions you can perform in response to a process terminating we
// can't let the thread that receives the raw signal be the one that performs
// those actions because we have no control over the state of that thread when
// the signal is received and will almost certainly get all sorts of unpleasant
// concurrency behavior, not limited to deadlocks.
class ProcessRegistry {
public:
  // Returns the singleton process registry instance, creating it if necessary.
  static ProcessRegistry *get();

  // Registers a child process with this registry.
  bool add(pid_t pid, NativeProcess *process);

private:
  // Create the registry including starting the signal dispatcher thread.
  ProcessRegistry();

  // Wait for the signal dispatcher to complete its current action and then stop
  // the thread and clean up this registry.
  ~ProcessRegistry();

  typedef platform_hash_map<pid_t, NativeProcess*> ProcessMap;

  // Entry-point for the signal dispatcher thread.
  void *run_signal_dispatcher();

  // Fetch notifications for any terminated children and notify the
  // corresponding process objects.
  bool dispatch_pending_terminations();

  // Installs child signal handlers.
  void install_signal_handler();

  // Removes the signal handler that was installed by install_signal_handler and
  // returns to the previous behavior.
  void uninstall_signal_handler();

  // Create a process registry and set it as the default, plus install the
  // child signal handlers.
  static void install();

  // Stop the current process registry.
  static opaque_t uninstall();

  // Function that conforms to the signature expected by the signal api.
  static void handle_signal_bridge(int signum, siginfo_t *info, void *context);

  // Does the actual work of handling a signal.
  bool handle_signal(int signum, siginfo_t *info, void *context);

  // Ensures that install gets called exactly once. This means that you can't
  // create a process registry more than once in the lifetime of the parent
  // process.
  static pthread_once_t install_once_;

  // The current process registry or NULL initially.
  static ProcessRegistry *current_;

  // Record of the signal handler that was active before we installed ours.
  struct sigaction prev_action_;

  // Mapping from pid to process.
  ProcessMap children_;

  // Mutex that guards the shutdown flag and children map. This isn't used by
  // incoming signals, only by the dispatcher thread itself and other threads
  // adding new processes.
  NativeMutex guard_;

  // Gets set to true when the dispatcher should stop running.
  volatile bool shutdown_;

  // Handle for the signal dispatcher thread.
  NativeThread dispatcher_;

  // Number of actions for the dispatcher to perform. An action can either be
  // to shut down or to process a pending signal, in particular as long as
  // shutdown is false this will be released every time a signal is added to the
  // pending list.
  NativeSemaphore action_count_;
};

class NativeProcess::PlatformData {
public:
  PlatformData();
  ~PlatformData();
  Drawbridge exited;
  pid_t pid;
};

NativeProcess::PlatformData::PlatformData()
  : pid(0) {
  exited.initialize();
}

NativeProcess::PlatformData::~PlatformData() {
  exited.pass();
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

bool NativeProcess::start(const char *executable, size_t argc, const char **argv) {
  CHECK_EQ("starting process already running", nsInitial, state);

  NativeProcessStart start(this);

  // Do the pre-flight work before doing the actual forking so we can report any
  // errors back in the parent process.
  if (!start.configure_file_descriptors() || !start.build_sub_environment())
    return false;

  platform_data_ = new NativeProcess::PlatformData();

  // Block SIGCHLD while we're forking. There's a potential race condition if
  // a child signal lands between fork() and adding to the process registry
  // which would cause the signal to be lost unless we explicitly make child
  // signals block for the duration.
  sigset_t block_mask;
  sigemptyset(&block_mask);
  sigaddset(&block_mask, SIGCHLD);
  errno = 0;
  if (sigprocmask(SIG_SETMASK, &block_mask, NULL) == -1)
    WARN("Call to sigprocmask failed: %s", strerror(errno));

  // Fork the child.
  pid_t fork_pid = fork();
  bool result = true;
  if (fork_pid == -1) {
    // Forking failed for some reason.
    WARN("Call to fork failed: %i", fork_pid);
    platform_data_->exited.lower();
    result = false;
  } else if (fork_pid > 0) {
    ProcessRegistry::get()->add(fork_pid, this);
    // We're in the parent so just record the child's pid and we're done.
    this->platform_data_->pid = fork_pid;
    this->state = nsRunning;
    result = start.parent_post_fork();
  } else {
    result = start.child_post_fork(executable, argc, argv);
  }

  // Unblock child signals, we're now all set to receive those again.
  errno = 0;
  if (sigprocmask(SIG_UNBLOCK, &block_mask, NULL) == -1)
    WARN("Call to sigprocmask failed: %s", strerror(errno));

  return result;
}

bool NativeProcess::wait_sync(Duration timeout) {
  CHECK_EQ("waiting for process not running", nsRunning, state);
  // Once the process terminates the drawbridge will be lowered.
  bool passed = platform_data_->exited.pass(timeout);
  if (passed)
    this->state = nsComplete;
  return passed;
}

bool NativeProcess::mark_terminated(int result) {
  this->result = result;
  bool lowered = platform_data_->exited.lower();
  if (!lowered)
    WARN("Failed to lower drawbridge for %lli", this->platform_data_->pid);
  return lowered;
}

int NativeProcess::exit_code() {
  CHECK_EQ("getting exit code of running process", nsComplete, state);
  return WEXITSTATUS(result);
}

bool ProcessRegistry::handle_signal(int signum, siginfo_t *info, void *context) {
  // Handling a signal is very easy: we just tell the dispatcher thread that
  // there is work to do and it will process any pending terminations. We don't
  // use the info at all because it can actually be misleading. Because SIGCHLDs
  // don't queue any signal may represent multiple children terminating and the
  // child that happens to be mentioned in the info is just one of them.
  return action_count_.release();
}

bool ProcessRegistry::add(pid_t pid, NativeProcess *child) {
  if (!guard_.lock())
    return false;
  children_[pid] = child;
  return guard_.unlock();
}

void ProcessRegistry::install() {
  CHECK_TRUE("multiple process registries", current_ == NULL);
  current_ = new ProcessRegistry();
  lifetime_t *default_lifetime = lifetime_get_default();
  CHECK_FALSE("no lifetime", default_lifetime == NULL);
  lifetime_atexit(default_lifetime, nullary_callback_new_0(ProcessRegistry::uninstall));
}

void ProcessRegistry::handle_signal_bridge(int signum, siginfo_t *info, void *context) {
  current_->handle_signal(signum, info, context);
}

void ProcessRegistry::install_signal_handler() {
  struct sigaction action;
  memset(&action, 0, sizeof(struct sigaction));
  action.sa_flags = (SA_SIGINFO | SA_RESTART);
  action.sa_sigaction = ProcessRegistry::handle_signal_bridge;
  sigaction(SIGCHLD, &action, &prev_action_);
}

void ProcessRegistry::uninstall_signal_handler() {
  sigaction(SIGCHLD, &prev_action_, NULL);
}

opaque_t ProcessRegistry::uninstall() {
  size_t child_count = current_->children_.size();
  if (child_count > 0)
    WARN("Exiting with %lli live child processes.", child_count);
  delete current_;
  current_ = NULL;
  return o0();
}

void *ProcessRegistry::run_signal_dispatcher() {
  while (true) {
    // Wait for the next action to run.
    if (!action_count_.acquire()) {
      WARN("Failed to get next action; aborting.");
      return NULL;
    }
    // If we're being asked to shut down do that, otherwise there'll be a signal
    // in the queue.
    if (shutdown_)
      break;
    // Acquire the guard since we're touching the state.
    dispatch_pending_terminations();
  }
  return NULL;
}

bool ProcessRegistry::dispatch_pending_terminations() {
  while (true) {
    int result = 0;
    errno = 0;
    // Wait for any child to terminate. This may or may not be the child we
    // were notified about, it doesn't make a difference: we know that one
    // has terminated and if others have too we might as well deal with those
    // at the same time.
    pid_t next = waitpid(-1, &result, WNOHANG);
    if (next == -1)
      break;
    // Get the child out of the mapping.
    if (!guard_.lock()) {
      WARN("Failed to lock process registry guard");
      return false;
    }
    ProcessMap::iterator iter = children_.find(next);
    if (iter == children_.end()) {
      // We don't know about this child; just skip it and keep going.
      if (!guard_.unlock()) {
        WARN("Failed to unlock process registry guard");
        return false;
      }
      continue;
    }
    // We know this process. First remove it from the children map while we hold
    // the lock.
    NativeProcess *process = iter->second;
    children_.erase(next);
    if (!guard_.unlock()) {
      WARN("Failed to unlock process registry guard");
      return false;
    }
    // Finally mark the process without holding any locks.
    process->mark_terminated(result);
  }
  return true;
}

ProcessRegistry::ProcessRegistry()
  : shutdown_(false)
  , action_count_(0) {
  guard_.initialize();
  action_count_.initialize();
  dispatcher_.set_callback(new_callback(&ProcessRegistry::run_signal_dispatcher, this));
  dispatcher_.start();
  install_signal_handler();
}

ProcessRegistry::~ProcessRegistry() {
  uninstall_signal_handler();
  guard_.lock();
  shutdown_ = true;
  guard_.unlock();
  action_count_.release();
  dispatcher_.join();
}

ProcessRegistry *ProcessRegistry::current_ = NULL;
pthread_once_t ProcessRegistry::install_once_ = PTHREAD_ONCE_INIT;

ProcessRegistry *ProcessRegistry::get() {
  pthread_once(&install_once_, install);
  return current_;
}
