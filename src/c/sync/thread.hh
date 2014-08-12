//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_THREAD_HH
#define _TCLIB_THREAD_HH

#include "stdc.h"
#include "callback.hh"

namespace tclib {

// An os-native thread.
class NativeThread {
public:
  typedef callback_t<void*(void)> run_callback_t;

  // Initialize a thread that calls the given callback when started. The thread
  // must be started explicitly by calling start().
  NativeThread(run_callback_t callback);
  ~NativeThread();

  // Starts this thread.
  void start();

  // Waits for this thread to finish, returning the result of the invocation
  // of the callback.
  void *join();

private:
  class Data;
  Data *data() { return data_; }
  Data *data_;
};

} // namespace tclib

#endif // _TCLIB_THREAD_HH
