//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <pthread.h>
#include <errno.h>

bool NativeMutex::platform_initialize() {
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  int result = pthread_mutex_init(&mutex, &attr);
  if (result == 0)
    return true;
  WARN("Call to pthread_mutex_init failed: %i (error: %s)", result, strerror(result));
  return false;
}

bool NativeMutex::platform_dispose() {
  int result = pthread_mutex_destroy(&mutex);
  if (result == 0)
    return true;
  WARN("Call to pthread_mutex_destroy failed: %i (error: %s)", result, strerror(result));
  return false;
}

bool NativeMutex::lock() {
  int result = pthread_mutex_lock(&mutex);
  if (result == 0)
    return true;
  WARN("Call to pthread_mutex_lock failed: %i (error: %s)", result, strerror(result));
  return false;
}

bool NativeMutex::try_lock() {
  int result = pthread_mutex_trylock(&mutex);
  if (result == 0)
    return true;
  if (result != EBUSY)
    // Busy means that it's already held which is a normal result so don't warn
    // on that.
    WARN("Call to pthread_mutex_trylock failed: %i (error: %s)", result, strerror(result));
  return false;
}

bool NativeMutex::unlock() {
  int result = pthread_mutex_unlock(&mutex);
  if (result == 0)
    return true;
  WARN("Call to pthread_mutex_unlock failed: %i (error: %s)", result, strerror(result));
  return false;
}
