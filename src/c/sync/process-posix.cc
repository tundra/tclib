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
  fat_bool_t configure_file_descriptors();
  fat_bool_t build_sub_environment();
  fat_bool_t parent_post_fork();
  fat_bool_t child_post_fork(utf8_t executable, size_t argc, utf8_t *argv);
  void remap_std_stream(stdio_stream_t stream, int old_fd);
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
// on to a helper thread which it owns. The helper thread processes all
// children that have died, including but not limited to the one that caused the
// signal. SIGCHLD doesn't queue so we have to assume that any one signal can
// covers multiple children that terminated but whose signals were lost behind
// the one we're now processing.
//
// The reason for dispatching from a helper thread is that unless we want to
// severely restrict actions you can perform in response to a process
// terminating we can't let the thread that receives the raw signal be the one
// that performs those actions because we have no control over the state of that
// thread when the signal is received and will almost certainly get all sorts of
// unpleasant concurrency behavior, not limited to deadlocks.
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
  opaque_t run_signal_dispatcher();

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
  // process. If this turns out to be a problem that's fixable.
  static pthread_once_t install_once_;

  // The current process registry or NULL initially. If there's a crash because
  // this was NULL unexpectedly it may be because the registry has been
  // uninstalled and attempted to be reinstalled.
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

  // Number of actions for the dispatcher to perform. If shutdown_ is true the
  // action will be to clean up and leave, otherwise it will be to process
  // child terminations.
  NativeSemaphore action_count_;
};

NativeProcessHandle::NativeProcessHandle()
  : id_(0) { }

NativeProcessStart::NativeProcessStart(NativeProcess *process)
  : process_(process) { }

fat_bool_t NativeProcessStart::configure_file_descriptors() {
  for (size_t i = 0; i < kStdioStreamCount; i++) {
    StreamRedirect redir = process_->stdio_[i];
    if (!redir.is_empty()) {
      if (!redir.prepare_launch())
        return F_FALSE;
      if (redir.remote_handle() == AbstractStream::kNullNakedFileHandle) {
        WARN("Invalid stdio stream %i", i);
        return F_FALSE;
      }
    }
  }
  return F_TRUE;
}

fat_bool_t NativeProcessStart::build_sub_environment() {
  if (process_->env_.empty())
    // Fast case if the sub-environment is identical to the caller's. In that
    // case we just leave it NULL and the child won't use it.
    return F_TRUE;

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

  return F_TRUE;
}

fat_bool_t NativeProcessStart::parent_post_fork() {
  fat_bool_t all_succeeded = F_TRUE;
  for (size_t i = 0; i < kStdioStreamCount; i++) {
    StreamRedirect redirect = process_->stdio_[i];
    if (!redirect.is_empty()) {
      fat_bool_t closed = redirect.parent_side_close();
      if (all_succeeded && !closed)
        // Capture the first failure.
        all_succeeded = closed;
    }
  }
  return all_succeeded;
}

void NativeProcessStart::remap_std_stream(stdio_stream_t stream, int old_fd) {
  StreamRedirect redirect = process_->stdio_[stream];
  if (redirect.is_empty())
    return;
  int new_fd = redirect.remote_handle();
  CHECK_TRUE("invalid fd", new_fd != AbstractStream::kNullNakedFileHandle);
  if (new_fd == old_fd)
    return;
  dup2(new_fd, old_fd);
  close(new_fd);
}

fat_bool_t NativeProcessStart::child_post_fork(utf8_t executable, size_t argc,
    utf8_t *argv) {
  // From here on we're in the child process. First redirect std streams if
  // necessary.
  remap_std_stream(siStdin, STDIN_FILENO);
  remap_std_stream(siStdout, STDOUT_FILENO);
  remap_std_stream(siStderr, STDERR_FILENO);

  for (size_t i = 0; i < kStdioStreamCount; i++) {
    StreamRedirect redirect = process_->stdio_[i];
    if (!(redirect.is_empty() || redirect.child_side_close()))
      return F_FALSE;
  }

  // Execv implicitly takes its environment from environ. The underlying data
  // would be disposed when the NativeProcessStart is but we'll never get to
  // that point and execv will copy the environment along the the arguments.
  // See https://www.gnu.org/software/libc/manual/html_node/Executing-a-File.html.
  if (!new_environ_.empty())
    environ = new_environ_.data();

  // Run the executable.
  char **args = new char *[argc + 2];
  args[0] = const_cast<char*>(executable.chars);
  for (size_t i = 0; i < argc; i++)
    args[i + 1] = const_cast<char*>(argv[i].chars);
  args[argc + 1] = NULL;
  execv(executable.chars, args);
  // If successful execv replaces the process so we'll never reach this point.
  // If it fails though we'll drop to here and we need to bail out immediately.
  // The error code signals an os api error. Clean up first to make valgrind
  // happy -- it doesn't matter since the process is going away anyway but it
  // simplifies things.
  delete[] args;
  exit(EX_OSERR);
  // This should never be reached.
  ERROR("Fell through exec, pid %i", getpid());
  return F_FALSE;
}

fat_bool_t PipeRedirector::prepare_launch(StreamRedirect *redirect) const {
  return F_TRUE;
}

fat_bool_t PipeRedirector::parent_side_close(StreamRedirect *redirect) const {
  // The child now has a clone of the remote handle so we can close this side.
  return F_BOOL(remote_side(redirect)->close());
}

fat_bool_t PipeRedirector::child_side_close(StreamRedirect *redirect) const {
  // We have a clone of the local handle which is really only useful to the
  // parent so we close it.
  return F_BOOL(local_side(redirect)->close());
}

fat_bool_t NativeProcess::start(utf8_t executable, size_t argc, utf8_t *argv) {
  CHECK_EQ("starting process already running", nsInitial, state);

  NativeProcessStart start(this);

  // Do the pre-flight work before doing the actual forking so we can report any
  // errors back in the parent process.
  if (!start.configure_file_descriptors() || !start.build_sub_environment())
    return F_FALSE;

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
  fat_bool_t result = F_TRUE;
  if (fork_pid == -1) {
    // Forking failed for some reason.
    WARN("Call to fork failed: %i", fork_pid);
    exit_code_.fulfill(-1);
    result = F_FALSE;
  } else if (fork_pid > 0) {
    ProcessRegistry::get()->add(fork_pid, this);
    // We're in the parent so just record the child's pid and we're done.
    this->handle()->set_process(fork_pid);
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

fat_bool_t NativeProcess::resume() {
  // Not supported.
  return F_FALSE;
}

fat_bool_t NativeProcessHandle::kill() {
  // Add support for this if we ever need it.
  return F_FALSE;
}

bool NativeProcess::mark_terminated(int result) {
  bool fulfilled = this->exit_code_.fulfill(WEXITSTATUS(result));
  if (!fulfilled)
    WARN("Failed to fulfill for %lli", this->handle()->guid());
  return fulfilled;
}

fat_bool_t NativeProcessHandle::start_inject_library(InjectRequest *request) {
  return F_FALSE;
}

fat_bool_t NativeProcessHandle::complete_inject_library(InjectRequest *request,
    Duration timeout) {
  // Starting injection can't succeed so definitely we don't want anyone to be
  // calling complete.
  UNREACHABLE("completing injection");
  return F_FALSE;
}

pid_t NativeProcessHandle::guid() {
  // The raw handle and id happen to be the same on posix, the pid.
  return id_;
}

fat_bool_t NativeProcessHandle::open(pid_t id) {
  id_ = id;
  return F_TRUE;
}

fat_bool_t NativeProcessHandle::close() {
  return F_TRUE;
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
  delete current_;
  current_ = NULL;
  return o0();
}

opaque_t ProcessRegistry::run_signal_dispatcher() {
  while (true) {
    // Wait for the next action to run.
    if (!action_count_.acquire()) {
      WARN("Failed to get next action; aborting.");
      return o0();
    }
    // If we're being asked to shut down do that, otherwise there'll be a signal
    // in the queue.
    if (shutdown_)
      break;
    // Acquire the guard since we're touching the state.
    dispatch_pending_terminations();
  }
  return o0();
}

bool ProcessRegistry::dispatch_pending_terminations() {
  while (true) {
    int result = 0;
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
  bool alls_well = true;
  alls_well = guard_.initialize() && alls_well;
  alls_well = action_count_.initialize() && alls_well;
  dispatcher_.set_callback(new_callback(&ProcessRegistry::run_signal_dispatcher, this));
  alls_well = dispatcher_.start() && alls_well;
  install_signal_handler();
  if (!alls_well)
    WARN("Process registry initialization failed. Somehow.");
}

ProcessRegistry::~ProcessRegistry() {
  if (!children_.empty())
    WARN("Exiting with %lli live child processes.", children_.size());
  uninstall_signal_handler();
  guard_.lock();
  shutdown_ = true;
  guard_.unlock();
  action_count_.release();
  dispatcher_.join(NULL);
}

ProcessRegistry *ProcessRegistry::current_ = NULL;
pthread_once_t ProcessRegistry::install_once_ = PTHREAD_ONCE_INIT;

ProcessRegistry *ProcessRegistry::get() {
  pthread_once(&install_once_, install);
  return current_;
}
