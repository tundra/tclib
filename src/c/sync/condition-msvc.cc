//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

fat_bool_t NativeCondition::platform_initialize() {
  InitializeConditionVariable(get_platform_condition(this));
  return F_TRUE;
}

fat_bool_t NativeCondition::platform_dispose() {
  // It doesn't appear to be necessary to dispose windows condition variables.
  return F_TRUE;
}

fat_bool_t NativeCondition::wait(NativeMutex *mutex, Duration timeout) {
  PCONDITION_VARIABLE cond = get_platform_condition(this);
  PCRITICAL_SECTION cs = get_platform_mutex(mutex);
  if (SleepConditionVariableCS(cond, cs, timeout.to_winapi_millis()))
    return F_TRUE;
  if (GetLastError() != ERROR_TIMEOUT)
    WARN("Call to SleepConditionVariableCS failed: %i", GetLastError());
  return F_FALSE;
}

fat_bool_t NativeCondition::wake_one() {
  WakeConditionVariable(get_platform_condition(this));
  return F_TRUE;
}

fat_bool_t NativeCondition::wake_all() {
  WakeAllConditionVariable(get_platform_condition(this));
  return F_TRUE;
}
