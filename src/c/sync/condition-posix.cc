//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <pthread.h>
#include <errno.h>

bool NativeCondition::platform_initialize() {
  int result = pthread_cond_init(&cond, NULL);
  if (result == 0)
    return true;
  WARN("Call to pthread_cond_init failed: %i (error: %s)", result, strerror(result));
  return false;
}

bool NativeCondition::platform_dispose() {
  int result = pthread_cond_destroy(&cond);
  if (result == 0)
    return true;
  WARN("Call to pthread_cond_destroy failed: %i (error: %s)", result, strerror(result));
  return false;
}
