//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_THREAD_HH
#define _TCLIB_THREAD_HH

#include "c/stdc.h"
#include "sync/sync.h"
#include "utils/callback.hh"

namespace tclib {

// An os-native thread.
class NativeThread {
public:
  typedef callback_t<void*(void)> run_callback_t;

  // Initialize a thread that calls the given callback when started. The thread
  // must be started explicitly by calling start().
  NativeThread(run_callback_t callback);

  // Initialize a thread without an initial callback. The set_callback method
  // must be used to set the callback before starting the thread.
  NativeThread();

  ~NativeThread();

  // Starts this thread. Returns true on success.
  bool start();

  // Waits for this thread to finish, returning the result of the invocation
  // of the callback.
  void *join();

  // If no callback was given at initialization this sets it to the given value.
  void set_callback(run_callback_t callback);

  // Returns the id of the current thread. The value is opaque and can only be
  // used for equality testing.
  static native_thread_id_t get_current_id();

  // Returns true iff the two given thread ids are identical.
  static bool ids_equal(native_thread_id_t a, native_thread_id_t b);

private:
  // Platform-specific start routine.
  bool platform_start();

  // Platform-specific start routine.
  bool platform_dispose();

  static PLATFORM_THREAD_ENTRY_POINT;

  // Callback to run on start.
  run_callback_t callback_;

  bool is_initialized_;
  // Platform-specific data.
  platform_thread_t thread_;
};

} // namespace tclib

#endif // _TCLIB_THREAD_HH
