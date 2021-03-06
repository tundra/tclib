//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <errno.h>
#include <pthread.h>

#include "utils/clock.hh"

fat_bool_t NativeCondition::platform_initialize() {
  int result = pthread_cond_init(&cond, NULL);
  if (result == 0)
    return F_TRUE;
  WARN("Call to pthread_cond_init failed: %i (error: %s)", result, strerror(result));
  return F_FALSE;
}

fat_bool_t NativeCondition::platform_dispose() {
  int result = pthread_cond_destroy(&cond);
  if (result == 0)
    return F_TRUE;
  WARN("Call to pthread_cond_destroy failed: %i (error: %s)", result, strerror(result));
  return F_FALSE;
}

fat_bool_t NativeCondition::wait(NativeMutex *mutex, Duration timeout) {
  int result;
  if (timeout.is_unlimited()) {
    result = pthread_cond_wait(&cond, &mutex->mutex);
  } else {
    NativeTime time = RealTimeClock::system()->time_since_epoch_utc() + timeout;
    struct timespec posix_time = time.to_posix();
    result = pthread_cond_timedwait(&cond, &mutex->mutex, &posix_time);
    if (result == ETIMEDOUT)
      return F_FALSE;
  }
  if (result == 0)
    return F_TRUE;
  WARN("Call to pthread_cond_wait failed: %i (error: %s)", result, strerror(result));
  return F_FALSE;
}

fat_bool_t NativeCondition::wake_one() {
  int result = pthread_cond_signal(&cond);
  if (result == 0)
    return F_TRUE;
  WARN("Call to pthread_cond_signal failed: %i (error: %s)", result, strerror(result));
  return F_FALSE;
}

fat_bool_t NativeCondition::wake_all() {
  int result = pthread_cond_broadcast(&cond);
  if (result == 0)
    return F_TRUE;
  WARN("Call to pthread_cond_broadcast failed: %i (error: %s)", result, strerror(result));
  return F_FALSE;
}
