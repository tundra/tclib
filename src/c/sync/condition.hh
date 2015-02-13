//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_CONDITION_HH
#define _TCLIB_CONDITION_HH

#include "c/stdc.h"
#include "sync/mutex.hh"

BEGIN_C_INCLUDES
#include "sync/condition.h"
END_C_INCLUDES

namespace tclib {

// An os-native condition variable.
class NativeCondition : public native_condition_t {
public:
  // Create a new uninitialized cpndition variable.
  NativeCondition();

  // Dispose this condition variable.
  ~NativeCondition();

  // Initialize this condition variable. Returns true iff initialization
  // succeeds.
  bool initialize();

  // Block this condition on the given mutex and release the mutex (which must
  // be held exactly once by the calling thread) atomically. While blocked the
  // thread may wake up spuriously without being explicitly woken.
  bool wait(NativeMutex *mutex);

  // Wake at least one thread that is waiting on this condition, if any are
  // waiting, but not necessarily all of them.
  bool wake_one();

  // Wake all threads waiting on this condition.
  bool wake_all();

private:
  // Platform-specific initialization.
  bool platform_initialize();

  // Platform-specific destruction.
  bool platform_dispose();
};

} // namespace tclib

#endif // _TCLIB_CONDITION_HH
