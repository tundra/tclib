//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_MUTEX_HH
#define _TCLIB_MUTEX_HH

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "sync/sync.h"
END_C_INCLUDES

namespace tclib {

// An os-native mutex.
class NativeMutex {
public:
  // Create a new uninitialized mutex.
  NativeMutex();

  // Dispose this mutex.
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

private:
  // Platform-specific initialization.
  bool platform_initialize();

  // Platform-specific destruction.
  bool platform_dispose();

  bool is_initialized_;
  platform_mutex_t mutex_;
};

} // namespace tclib

#endif // _TCLIB_MUTEX_HH
