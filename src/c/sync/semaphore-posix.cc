//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <semaphore.h>
#include <errno.h>

// Posix semaphores are different from the other concurrency primitives in that
// they return error codes through errno instead of their result values which
// will always be -1 on errors. It's okay though, errno should be thread safe.

bool NativeSemaphore::platform_initialize() {
  int result = sem_init(&sema_, false, initial_count_);
  if (result == 0)
    return true;
  WARN("Call to sem_init failed: %i (error: %s)", result, strerror(errno));
  return false;
}

bool NativeSemaphore::platform_dispose() {
  int result = sem_destroy(&sema_);
  if (result != 0)
    WARN("Call to sem_destroy failed: %i (error: %s)", result, strerror(errno));
  return result == 0;
}

bool NativeSemaphore::acquire(duration_t timeout) {
  int result;
  if (duration_is_unlimited(timeout)) {
    result = sem_wait(&sema_);
  } else {
    // First grab the current time.
    struct timespec spec;
    if (clock_gettime(CLOCK_REALTIME, &spec) == -1)
      return false;
    uint64_t sec = spec.tv_sec;
    uint64_t nsec = spec.tv_nsec;
    duration_add_to_timespec(timeout, &sec, &nsec);
    spec.tv_sec = sec;
    spec.tv_nsec = nsec;
    result = sem_timedwait(&sema_, &spec);
  }
  if (result == 0)
    return true;
  if (errno != ETIMEDOUT)
    // Timing out is fine so only warn if it's a different error.
    WARN("Call to sem_wait failed: %i (error: %s)", result, strerror(errno));
  return false;
}

bool NativeSemaphore::try_acquire() {
  int result = sem_trywait(&sema_);
  if (result == 0)
    return true;
  if (errno != EAGAIN)
    // EAGAIN indicates that nothing went wrong as such, the semaphore is just
    // zero.
    WARN("Call to sem_trywait failed: %i (error: %i %s)", result, strerror(errno));
  return false;
}

bool NativeSemaphore::release() {
  int result = sem_post(&sema_);
  if (result == 0)
    return true;
  WARN("Call to sem_post failed: %i (error: %s)", result, strerror(errno));
  return false;
}
