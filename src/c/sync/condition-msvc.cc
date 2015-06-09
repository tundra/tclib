//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

bool NativeCondition::platform_initialize() {
  InitializeConditionVariable(get_platform_condition(this));
  return true;
}

bool NativeCondition::platform_dispose() {
  // It doesn't appear to be necessary to dispose windows condition variables.
  return true;
}

bool NativeCondition::wait(NativeMutex *mutex, Duration timeout) {
  PCONDITION_VARIABLE cond = get_platform_condition(this);
  PCRITICAL_SECTION cs = get_platform_mutex(mutex);
  dword_t millis = timeout.is_unlimited() ? INFINITE : timeout.to_millis();
  if (SleepConditionVariableCS(cond, cs, millis))
    return true;
  WARN("Call to SleepConditionVariableCS failed: %i", GetLastError());
  return false;
}

bool NativeCondition::wake_one() {
  WakeConditionVariable(get_platform_condition(this));
  return true;
}

bool NativeCondition::wake_all() {
  WakeAllConditionVariable(get_platform_condition(this));
  return true;
}
