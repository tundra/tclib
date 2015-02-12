//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Mach looks like it supports posix-style unnamed semaphores through the posix
// api but actually it doesn't, the calls fail.

bool NativeSemaphore::platform_initialize() {
  kern_return_t result = semaphore_create(mach_task_self(), &sema,
      SYNC_POLICY_FIFO, initial_count);
  if (result == KERN_SUCCESS)
    return true;
  WARN("Call to semaphore_create failed: %i", result);
  return false;
}

bool NativeSemaphore::platform_dispose() {
  kern_return_t result = semaphore_destroy(mach_task_self(), sema);
  if (result != KERN_SUCCESS) {
    WARN("Call to semaphore_destroy failed: %i", result);
    return false;
  }
  return true;
}

bool NativeSemaphore::acquire(duration_t timeout) {
  kern_return_t result;
  if (duration_is_unlimited(timeout)) {
    result = semaphore_wait(sema);
  } else {
    // Unlike sem_timedwait on posix, semaphore_timedwait takes the time to
    // wait, not the deadline, so constructing the timespec is simpler here.
    uint64_t sec = 0;
    uint64_t nsec = 0;
    duration_add_to_timespec(timeout, &sec, &nsec);
    mach_timespec time;
    time.tv_sec = sec;
    time.tv_nsec = nsec;
    result = semaphore_timedwait(sema, time);
  }
  if (result == KERN_SUCCESS)
    return true;
  if (result != KERN_OPERATION_TIMED_OUT)
    WARN("Waiting for semaphore failed: %i", result);
  return false;
}

bool NativeSemaphore::try_acquire() {
  mach_timespec time;
  time.tv_sec = 0;
  time.tv_nsec = 0;
  kern_return_t result = semaphore_timedwait(sema, time);
  if (result == KERN_SUCCESS)
    return true;
  if (result != KERN_OPERATION_TIMED_OUT)
    // Not being able to acquire will look like a timeout so don't warn on that,
    // we expect it.
    WARN("Call to semaphore_timedwait failed: %i", result);
  return false;
}

bool NativeSemaphore::release() {
  kern_return_t result = semaphore_signal(sema);
  if (result == KERN_SUCCESS)
    return true;
  WARN("Call to semaphore_signal failed: %i", result);
  return false;
}
