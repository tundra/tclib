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

  // The largest possible size of the underlying data. Public for testing only.
  static const size_t kMaxDataSize = WORD_SIZE * 4;

  // Returns the size in bytes of a data object.
  static size_t get_data_size();

private:
  // The raw memory that will hold the platform-specific data.
  uint8_t data_memory_[kMaxDataSize];

  run_callback_t callback_;

  // Pointer to the initialized platform-specific data.
  class Data;
  Data *data_;
};

} // namespace tclib

#endif // _TCLIB_THREAD_HH
