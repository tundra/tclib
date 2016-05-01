//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PROCESS_HH
#define _TCLIB_PROCESS_HH

#include "c/stdc.h"

#include <string>

#include "async/promise.hh"
#include "c/stdvector.hh"
#include "io/stream.hh"
#include "utils/fatbool.hh"

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
  fat_bool_t prepare_launch();
  fat_bool_t parent_side_close();
  fat_bool_t child_side_close();

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
  virtual fat_bool_t prepare_launch(StreamRedirect *redirect) const = 0;

  // Called after the child has been successfully spawned to clean up the parent
  // side of the redirect.
  virtual fat_bool_t parent_side_close(StreamRedirect *redirect) const = 0;

  // Called after the child has been successfully spawned to clean up the child
  // side of the redirect.
  virtual fat_bool_t child_side_close(StreamRedirect *redirect) const = 0;
};

// A pipe-based redirector. This takes ownership of the pipe and will close the
// in- and outgoing handles appropriately. When the child process exits the
// pipe will be dead.
class PipeRedirector : public StreamRedirector {
public:
  PipeRedirector(pipe_direction_t direction);
  virtual naked_file_handle_t remote_handle(StreamRedirect *redirect) const;
  virtual fat_bool_t prepare_launch(StreamRedirect *redirect) const;
  virtual fat_bool_t parent_side_close(StreamRedirect *redirect) const;
  virtual fat_bool_t child_side_close(StreamRedirect *redirect) const;

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

// A handle to an existing process, either created by this one or by someone
// else.
class NativeProcessHandle {
public:
  NativeProcessHandle();

  // Internal state about an injection optionally used by the implementation.
  class InjectState;

  // State associated with injecting a dll into a process.
  class InjectRequest {
  public:
    InjectRequest(utf8_t path);
    void set_connector(utf8_t name, blob_t data_in, blob_t data_out);
    utf8_t path() { return path_; }
    utf8_t connector_name() { return connector_name_; }
    blob_t data_in() { return data_in_; }
    blob_t data_out() { return data_out_; }
    InjectState *state() { return state_; }
    void set_state(InjectState *state) { state_ = state; }
  private:
    utf8_t path_;
    utf8_t connector_name_;
    blob_t data_in_;
    blob_t data_out_;
    InjectState *state_;
  };

  void set_id(platform_process_t value) { id_ = value; }

  // Starts the process of injecting the requested library but doesn't wait for
  // it to complete. See inject_library for details.
  fat_bool_t start_inject_library(InjectRequest *request);

  // Waits for the injection process to complete.
  fat_bool_t complete_inject_library(InjectRequest *request, Duration timeout = Duration::unlimited());

private:
  platform_process_t id() { return id_; }
  platform_process_t id_;
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
  fat_bool_t start(utf8_t executable, size_t argc, utf8_t *argv);

  // If this thread was started in suspended mode resumes execution.
  fat_bool_t resume();

  // Attempt to forcefully destroy this process.
  fat_bool_t kill();

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

  // Sets this process' flags. The flags come from native_process_flags_t.
  void set_flags(int32_t value) { flags_ = value; }

  // On windows, loads a dll into a suspended process. Returns true if
  // successful and false otherwise, including on linux where this is just not
  // supported.
  //
  // If a nonempty connect_name is passed it will be used as the name of an
  // exported function within the dll to call to connect with the caller; if the
  // method doesn't exist inject_library will fail. The connect function will
  // be passed a copy of the data in blob_in and a piece of scratch memory the
  // same size as blob_out, whose contents will be copied back into blob_out
  // once the call has completed.
  fat_bool_t inject_library(NativeProcessHandle::InjectRequest *request);

  // Starts the process of injecting the requested library but doesn't wait for
  // it to complete. See inject_library for details. Similar to the method of
  // the same name on NativeProcessHandle but does more checking.
  fat_bool_t start_inject_library(NativeProcessHandle::InjectRequest *request);

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

  // Returns a handle for this process; only valid after the process has been
  // started.
  NativeProcessHandle *handle() { return &handle_; }

  // Called asynchronously when the system notices that the process is done
  // running.
  ONLY_GCC(bool mark_terminated(int result);)
  ONLY_MSVC(void mark_terminated(bool timer_or_wait_fired);)

  // Given a function pointer, if it points to a jump thunk because of
  // incremental linking under MSVC returns a pointer to the actual destination.
  // If the function is not a jump thunk just returns the function itself.
  static void *resolve_jump_thunk(void *fun);

  // Can be used to test whether suspend/resume works on this platform.
  static const bool kCanSuspendResume = kIsMsvc;

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
  NativeProcessHandle handle_;
};

} // namespace tclib

#endif // _TCLIB_PROCESS_HH
