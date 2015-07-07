//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PROCESS_HH
#define _TCLIB_PROCESS_HH

#include "c/stdc.h"

#include <string>

#include "async/promise.hh"
#include "c/stdvector.hh"
#include "io/stream.hh"

BEGIN_C_INCLUDES
#include "sync/sync.h"
#include "sync/process.h"
END_C_INCLUDES

struct stream_redirect_t { };
struct native_process_t { };

namespace tclib {

class NativePipe;

// Encapsulates the behavior of redirecting input/output to a process. The
// implementations and patterns of use are somewhat platform dependent so read
// the platform-independent documentation with a grain of salt, you kind of have
// to know what's going on on the platform to know how it really works.
class StreamRedirect : public stream_redirect_t {
public:
  // No-op destructor.
  virtual ~StreamRedirect() { }

  // Returns the file handle to export to the child process.
  virtual naked_file_handle_t remote_handle() = 0;

  // Perform any work that needs to be done before creating the child.
  virtual bool prepare_launch() = 0;

  // Called after the child has been successfully spawned to clean up the parent
  // side of the redirect.
  virtual bool parent_side_close() = 0;

  // Called after the child has been successfully spawned to clean up the child
  // side of the redirect.
  virtual bool child_side_close() = 0;
};

// A pipe-based redirect. This takes ownership of the pipe and will close the
// in- and outgoing handles appropriately. When the child process exits the
// pipe will be dead.
class PipeRedirect : public StreamRedirect {
public:
  PipeRedirect(NativePipe *pipe, pipe_direction_t direction);
  PipeRedirect();
  virtual naked_file_handle_t remote_handle();
  virtual bool prepare_launch();
  virtual bool parent_side_close();
  virtual bool child_side_close();
  void set_pipe(NativePipe *pipe, pipe_direction_t direction);

private:
  NativePipe *pipe_;
  pipe_direction_t direction_;

  // Does this redirect output or input?
  bool is_output() { return direction_ == pdOut; }

  // The remote stream, the stream that will be passed on to the child.
  AbstractStream *remote_side();

  // The local stream, the stream the current process will use to interact with
  // the child.
  AbstractStream *local_side();
};

// An os-native process.
class NativeProcess: public native_process_t {
public:
  // Current running state of a native process. Used to control internal behavior.
  typedef enum {
    nsInitial,
    nsRunning,
    nsCouldntCreate,
    nsComplete
  } state_t;

  // Create a new uninitialized process.
  NativeProcess();

  // Dispose this process.
  ~NativeProcess();

  // Start this process running. This will return immediately after spawning
  // the child process, there is no guarantee that the executable is started or
  // indeed completes successfully.
  bool start(utf8_t executable, size_t argc, utf8_t *argv);

  // Adds an environment mapping to the set visible to the process. The process
  // copies the key and value so they can be released immediately after this
  // call. Note that if the key includes an equals sign it will cause not just
  // that binding to be created but also permutations around the the equals
  // sign. For instance, if you do set_env("A=B", "c=d") you will get a binding
  // for ("A=B", "c=d") but also ("A", "B=c=d"), and ("A=B=c", "d"). This is not
  // necessarily what you want -- it would be nice just from an orthogonality
  // viewpoint to be able to pass values with equals signs in them without
  // getting funky behavior, but it's not clear that there's a way to do that
  bool set_env(utf8_t key, utf8_t value);

  // Sets the stream to use as standard input for the running process. Must be
  // called before starting the process.
  void set_stdin(StreamRedirect *redirect) {
    stdin_ = redirect;
  }

  // Sets the stream to use as standard output for the running process. Must be
  // called before starting the process.
  void set_stdout(StreamRedirect *redirect) {
    stdout_ = redirect;
  }

  // Sets the stream to use as standard error for the running process. Must be
  // called before starting the process.
  void set_stderr(StreamRedirect *redirect) {
    stderr_ = redirect;
  }

  // Wait synchronously for this process to terminate.
  bool wait_sync(Duration timeout = Duration::unlimited());

  // Returns a promise for the process' exit code. This will be resolved at some
  // point after the process terminates.
  promise_t<int> exit_code() { return exit_code_; }

  // Returns the same promise as the exit_code method but viewed as the C
  // promise type. The result is valid until the process object is destroyed.
  opaque_promise_t *opaque_exit_code();

  // Called asynchronously when the system notices that the process is done
  // running.
  ONLY_GCC(bool mark_terminated(int result);)
  ONLY_MSVC(void mark_terminated(bool timer_or_wait_fired);)

private:
  class PlatformData;
  friend class NativeProcessStart;

  state_t state;
  PlatformData *platform_data_;
  sync_promise_t<int> exit_code_;
  opaque_promise_t *opaque_exit_code_;
  StreamRedirect *stdin_;
  StreamRedirect *stdout_;
  StreamRedirect *stderr_;
  std::vector<std::string> env_;
};

} // namespace tclib

#endif // _TCLIB_PROCESS_HH
