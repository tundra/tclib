//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_SEMAPHORE_HH
#define _TCLIB_SEMAPHORE_HH

#include "c/stdc.h"

#include "utils/duration.hh"
#include "utils/fatbool.hh"

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
  fat_bool_t initialize();

  // Attempt to acquire a permit from this semaphore, blocking up to the given
  // duration if necessary.
  fat_bool_t acquire(Duration timeout = Duration::unlimited());

  // Attempt to acquire a permit from this semaphore but will not wait if no
  // permits are available. Shorthand for acquire(duration_instant()).
  fat_bool_t try_acquire();

  // Release a permit to this semaphore.
  fat_bool_t release();

private:
  // Platform-specific initialization.
  fat_bool_t platform_initialize();

  // Platform-specific destruction.
  fat_bool_t platform_dispose();
};

} // namespace tclib

#endif // _TCLIB_SEMAPHORE_HH
