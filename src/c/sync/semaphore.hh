//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_SEMAPHORE_HH
#define _TCLIB_SEMAPHORE_HH

#include "stdc.h"

namespace tclib {

// An os-native mutex.
class NativeSemaphore {
public:
  // Create a new uninitialized semaphore with an initial count of 1.
  NativeSemaphore();

  // Create a new uninitialized semaphore with the given initial count.
  NativeSemaphore(uint32_t initial_count);

  // Dispose this semaphore.
  ~NativeSemaphore();

  // Sets the initial count to the given value. Only has an effect if called
  // before the semaphore has been initialized.
  void set_initial_count(uint32_t value);

  // Initialize this semaphore, returning true on success.
  bool initialize();

  // Attempt to acquire a permit from this semaphore, blocking until one if
  // available if necessary.
  bool acquire();

  // Attempt to acquire a permit from this semaphore but will not wait if no
  // permits are available.
  bool try_acquire();

  // Release a permit to this semaphore.
  bool release();

  // The largest possible size of the underlying data. Public for testing only.
  static const size_t kMaxDataSize = WORD_SIZE * 4;

  // Returns the size in bytes of a data object.
  static size_t get_data_size();

private:
  // The raw memory that will hold the platform-specific data.
  uint8_t data_memory_[kMaxDataSize];

  uint32_t initial_count_;

  // Pointer to the initialized platform-specific data.
  class Data;
  Data *data_;
};

} // namespace tclib

#endif // _TCLIB_SEMAPHORE_HH
