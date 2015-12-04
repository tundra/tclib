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

struct stream_redirector_t { };
struct native_process_t { };

namespace tclib {

class NativePipe;
class StreamRedirector;

// Helper class that controls how an I/O device is used as a standard stream
// connected to a process. Usually it's not just a matter of connecting a
// stream to stdout, say, some extra management needs to be done based on the
// type of the input. For instance, with pipes you need to connect one end and
// close the other.
//
// The structure of this is a little complicated, that's because we need some
// extra behavior somewhere to deal with processes but it doesn't really belong
// on the streams themselves. Also, it's a lot of maintenance to allocate and
// manage the lifetimes of separate objects just to deal with this. So instead
// there's a few shared "redirectors" which take care of the behavior and then
// the data to be operated on, for instance the pipe, is passed to it.
class StreamRedirect : public stream_redirect_t {
public:
  StreamRedirect(const StreamRedirector *redirector, void *data);

  // Converts a C redirect to a C++ one.
  StreamRedirect(stream_redirect_t c_redirect);

  // Initializes an empty stream redirect.
  StreamRedirect();

  // Does this redirect have contents or has it been left empty?
  bool is_empty() { return redirector_ == NULL; }

  // See the corresponding methods on StreamRedirector.
  naked_file_handle_t remote_handle();
  bool prepare_launch();
  bool parent_side_close();
  bool child_side_close();

private:
  // Yields the redirector viewed as a C++ object.
  const StreamRedirector *redirector();
};

// Encapsulates the behavior of redirecting input/output to a process. The
// implementations and patterns of use are somewhat platform dependent so read
// the platform-independent documentation with a grain of salt, you kind of have
// to know what's going on on the platform to know how it really works.
class StreamRedirector : public stream_redirector_t {
public:
  // No-op destructor.
  virtual ~StreamRedirector() { }

  // Returns the file handle to export to the child process.
  virtual naked_file_handle_t remote_handle(StreamRedirect *redirect) const = 0;

  // Perform any work that needs to be done before creating the child.
  virtual bool prepare_launch(StreamRedirect *redirect) const = 0;

  // Called after the child has been successfully spawned to clean up the parent
  // side of the redirect.
  virtual bool parent_side_close(StreamRedirect *redirect) const = 0;

  // Called after the child has been successfully spawned to clean up the child
  // side of the redirect.
  virtual bool child_side_close(StreamRedirect *redirect) const = 0;
};

// A pipe-based redirector. This takes ownership of the pipe and will close the
// in- and outgoing handles appropriately. When the child process exits the
// pipe will be dead.
class PipeRedirector : public StreamRedirector {
public:
  PipeRedirector(pipe_direction_t direction);
  virtual naked_file_handle_t remote_handle(StreamRedirect *redirect) const;
  virtual bool prepare_launch(StreamRedirect *redirect) const;
  virtual bool parent_side_close(StreamRedirect *redirect) const;
  virtual bool child_side_close(StreamRedirect *redirect) const;

private:
  pipe_direction_t direction_;

  // Returns the given redirect's pipe.
  NativePipe *pipe(StreamRedirect *redirect) const { return (NativePipe*) o2p(redirect->o_data_); }

  // Does this redirect output or input?
  bool is_output() const { return direction_ == pdOut; }

  // The remote stream, the stream that will be passed on to the child.
  AbstractStream *remote_side(StreamRedirect *redirect) const;

  // The local stream, the stream the current process will use to interact with
  // the child.
  AbstractStream *local_side(StreamRedirect *redirect) const;
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

  // If this thread was started in suspended mode resumes execution.
  bool resume();

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

  // Returns the flags currently set on this process.
  int32_t flags() { return flags_; }

  // Sets this process' flags.
  void set_flags(int32_t value) { flags_ = value; }

  // On windows, loads a dll into a suspended process. Returns true if
  // successful and false otherwise, including on linux where this is just not
  // supported.
  bool inject_library(utf8_t path, utf8_t connect_name, blob_t blob_in, blob_t *blob_out);

  bool inject_library(utf8_t path);

  // Specifies that the given redirect should be used for the given stream. Must
  // be called before starting the process.
  void set_stream(stdio_stream_t stream, StreamRedirect redirect) {
    stdio_[stream] = redirect;
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

  PlatformData *platform_data() { return platform_data_; }

  state_t state;
  PlatformData *platform_data_;
  sync_promise_t<int> exit_code_;
  opaque_promise_t *opaque_exit_code_;
  StreamRedirect stdio_[kStdioStreamCount];
  std::vector<std::string> env_;
  int32_t flags_;
};

} // namespace tclib

#endif // _TCLIB_PROCESS_HH
