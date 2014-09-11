//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_MUTEX_HH
#define _TCLIB_MUTEX_HH

#include "stdc.h"

namespace tclib {

// An os-native mutex.
class NativeMutex {
public:
  // Create a new uninitialized mutex.
  NativeMutex();
  ~NativeMutex();

  // Initialize this mutex. Returns true iff initialization succeeds.
  bool initialize();

  // Lock this mutex. If it's already held by a different thread we'll wait for
  // it to be released. If it's already held by this thread that's fine, we'll
  // lock it again.
  bool lock();

  // Lock this mutex. If it's already held don't wait but return false
  // immediately.
  bool try_lock();

  // Unlock this mutex. Only the thread that holds this mutex will be allowed
  // to unlock it; if another thread tries the result will be undefined. Or
  // actually it will probably be well-defined for each platform (for instance,
  // on posix it will succeed, on windows it will fail) but across platforms
  // you shouldn't depend on any particular behavior.
  bool unlock();

  // The largest possible size of the underlying data. Public for testing only.
  static const size_t kMaxDataSize = WORD_SIZE * 8;

  // Returns the size in bytes of a data object.
  static size_t get_data_size();

private:
  // The raw memory that will hold the platform-specific data.
  uint8_t data_memory_[kMaxDataSize];

  // Pointer to the initialized platform-specific data.
  class Data;
  Data *data_;
};

} // namespace tclib

#endif // _TCLIB_MUTEX_HH
