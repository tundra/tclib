//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <semaphore.h>
#include <errno.h>

#include "utils/clock.hh"

// Posix semaphores are different from the other concurrency primitives in that
// they return error codes through errno instead of their result values which
// will always be -1 on errors. It's okay though, errno should be thread safe.

bool NativeSemaphore::platform_initialize() {
  int result = sem_init(&sema, false, initial_count);
  if (result == 0)
    return true;
  WARN("Call to sem_init failed: %i (error: %s)", result, strerror(errno));
  return false;
}

bool NativeSemaphore::platform_dispose() {
  int result = sem_destroy(&sema);
  if (result != 0)
    WARN("Call to sem_destroy failed: %i (error: %s)", result, strerror(errno));
  return result == 0;
}

bool NativeSemaphore::acquire(Duration timeout) {
  int result;
  errno = 0;
  if (timeout.is_unlimited()) {
    result = sem_wait(&sema);
  } else if (timeout.is_instant()) {
    result = sem_trywait(&sema);
  } else {
    NativeTime current = RealTimeClock::system()->time_since_epoch_utc();
    NativeTime deadline = current + timeout;
    result = sem_timedwait(&sema, &deadline.to_platform());
  }
  if (result == 0)
    return true;
  if (errno != ETIMEDOUT && errno != EAGAIN)
    // Timing out or unavailability is fine so only warn if it's a different
    // error.
    WARN("Waiting for semaphore failed: %i (error: %s)", result, strerror(errno));
  return false;
}

bool NativeSemaphore::release() {
  int result = sem_post(&sema);
  if (result == 0)
    return true;
  WARN("Call to sem_post failed: %i (error: %s)", result, strerror(errno));
  return false;
}
