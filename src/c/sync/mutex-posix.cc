//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <pthread.h>
#include <errno.h>

fat_bool_t NativeMutex::platform_initialize() {
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  int result = pthread_mutex_init(&mutex, &attr);
  if (result == 0)
    return F_TRUE;
  WARN("Call to pthread_mutex_init failed: %i (error: %s)", result, strerror(result));
  return F_FALSE;
}

fat_bool_t NativeMutex::platform_dispose() {
  int result = pthread_mutex_destroy(&mutex);
  if (result == 0)
    return F_TRUE;
  WARN("Call to pthread_mutex_destroy failed: %i (error: %s)", result, strerror(result));
  return F_FALSE;
}

fat_bool_t NativeMutex::lock(Duration timeout) {
  int result;
  if (timeout.is_unlimited()) {
    result = pthread_mutex_lock(&mutex);
  } else if (timeout.is_instant()) {
    result = pthread_mutex_trylock(&mutex);
  } else {
    CHECK_TRUE("nontrivial timeout not supported", false);
    return F_FALSE;
  }
  if (result == 0)
    return F_TRUE;
  // Busy means that it's already held which is a normal result so don't warn
  // on that.
  if (result != EBUSY)
    WARN("Call to pthread_mutex_lock failed: %i (error: %s)", result, strerror(result));
  return F_FALSE;
}

fat_bool_t NativeMutex::unlock() {
  int result = pthread_mutex_unlock(&mutex);
  if (result == 0)
    return F_TRUE;
  WARN("Call to pthread_mutex_unlock failed: %i (error: %s)", result, strerror(result));
  return F_FALSE;
}
