//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_MUTEX_HH
#define _TCLIB_MUTEX_HH

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "sync/sync.h"
#include "sync/mutex.h"
END_C_INCLUDES

namespace tclib {

// An os-native mutex.
class NativeMutex : public native_mutex_t {
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
  // to unlock it. If another thread tries to unlock it there are two possible
  // behaviors depending on which platform you're on. On some it will be caught
  // and unlocking will return false, after which the mutex is still in a valid
  // state. On others the behavior and return value will be undefined and the
  // mutex may be left broken. If checks_consistency() returns true the first
  // will be the case, otherwise it will be the second.
  bool unlock();

  // Returns true if unlocking while the mutex isn't held causes unlock to
  // return false. If this returns false the discipline is implicit and
  // unlocking inconsistently may leave the mutex permanently broken. Since it's
  // assumed that locks are used consistently in production code this is for
  // testing obviously.
  static bool checks_consistency() { return kPlatformMutexChecksConsistency; }

private:
  // Platform-specific initialization.
  bool platform_initialize();

  // Platform-specific destruction.
  bool platform_dispose();
};

} // namespace tclib

#endif // _TCLIB_MUTEX_HH
