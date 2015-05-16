//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PROCESS_HH
#define _TCLIB_PROCESS_HH

#include "c/stdc.h"

#include "io/stream.hh"

BEGIN_C_INCLUDES
#include "sync/sync.h"
#include "sync/process.h"
END_C_INCLUDES

namespace tclib {

// An os-native process.
class NativeProcess : public native_process_t {
public:
  // Create a new uninitialized process.
  NativeProcess();

  // Dispose this process.
  ~NativeProcess();

  // Start this process running. This will return immediately after spawning
  // the child process, there is no guarantee that the executable is started or
  // indeed completes successfully.
  bool start(const char *executable, size_t argc, const char **argv);

  // Sets the stream to use as standard output for the running process. Must be
  // called before starting the process.
  void set_stdout(OutStream *value) { stdout_ = value; }

  // Wait for this process, which must already have been started, to complete.
  // Returns true iff waiting succeeded. The process must have been started.
  bool wait();

  // Returns the process' exit code. The process must have been started and
  // waited on.
  int exit_code();

private:
  friend class NativeProcessStart;

  // Platform-specific initialization.
  void platform_initialize();

  // Platform-specific destruction.
  void platform_dispose();

  OutStream *stdout_;
};

} // namespace tclib

#endif // _TCLIB_PROCESS_HH
