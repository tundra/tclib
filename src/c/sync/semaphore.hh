//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_SEMAPHORE_HH
#define _TCLIB_SEMAPHORE_HH

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "sync/sync.h"
#include "sync/semaphore.h"
END_C_INCLUDES

namespace tclib {

// An os-native mutex.
class NativeSemaphore : public native_semaphore_t {
public:
  // Create a new uninitialized semaphore with an initial count of 1.
  NativeSemaphore();

  // Create a new uninitialized semaphore with the given initial count.
  NativeSemaphore(uint32_t initial_count);

  // Dispose this semaphore.
  ~NativeSemaphore();

  // Sets the initial count to the given value. Only has an effect if called
  // before the semaphore has been initialized.
  void set_initial_count(uint32_t value) { initial_count = value; }

  // Initialize this semaphore, returning true on success.
  bool initialize();

  // Attempt to acquire a permit from this semaphore, blocking up to the given
  // duration if necessary.
  bool acquire(duration_t timeout = duration_unlimited());

  // Attempt to acquire a permit from this semaphore but will not wait if no
  // permits are available.
  bool try_acquire();

  // Release a permit to this semaphore.
  bool release();

private:
  // Platform-specific initialization.
  bool platform_initialize();

  // Platform-specific destruction.
  bool platform_dispose();
};

} // namespace tclib

#endif // _TCLIB_SEMAPHORE_HH
