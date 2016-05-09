//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_CONDITION_HH
#define _TCLIB_CONDITION_HH

#include "c/stdc.h"

#include "sync/mutex.hh"
#include "utils/duration.hh"

BEGIN_C_INCLUDES
#include "sync/condition.h"
END_C_INCLUDES

namespace tclib {

// An os-native condition variable.
class NativeCondition : public native_condition_t {
public:
  // Create a new uninitialized condition variable.
  NativeCondition();

  // Dispose this condition variable.
  ~NativeCondition();

  // Initialize this condition variable. Returns true iff initialization
  // succeeds.
  fat_bool_t initialize();

  // Block this condition on the given mutex and release the mutex (which must
  // be held exactly once by the calling thread) atomically. While blocked the
  // thread may wake up spuriously without being explicitly woken.
  fat_bool_t wait(NativeMutex *mutex, Duration timeout = Duration::unlimited());

  // Wake at least one thread that is waiting on this condition, if any are
  // waiting, but not necessarily all of them.
  fat_bool_t wake_one();

  // Wake all threads waiting on this condition.
  fat_bool_t wake_all();

private:
  // Platform-specific initialization.
  fat_bool_t platform_initialize();

  // Platform-specific destruction.
  fat_bool_t platform_dispose();
};

} // namespace tclib

#endif // _TCLIB_CONDITION_HH
