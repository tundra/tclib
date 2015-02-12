//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_CONDITION_HH
#define _TCLIB_CONDITION_HH

#include "c/stdc.h"

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

private:
  // Platform-specific initialization.
  bool platform_initialize();

  // Platform-specific destruction.
  bool platform_dispose();
};

} // namespace tclib

#endif // _TCLIB_CONDITION_HH
