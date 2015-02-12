//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

static PCONDITION_VARIABLE wincond(NativeCondition *cond) {
  // There should be code elsewhere that ensures that the cond field is large
  // enough to hold a native condition variable.
  return reinterpret_cast<PCONDITION_VARIABLE>(&cond->cond);
}

bool NativeCondition::platform_initialize() {
  InitializeConditionVariable(wincond(this));
  return true;
}

bool NativeCondition::platform_dispose() {
  return true;
}
