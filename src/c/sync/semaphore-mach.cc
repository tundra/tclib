//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Mach looks like it supports posix-style unnamed semaphores through the posix
// api but actually it doesn't, the calls fail.

#include "utils/clock.hh"

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

bool NativeSemaphore::acquire(Duration timeout) {
  kern_return_t result;
  if (timeout.is_unlimited()) {
    result = semaphore_wait(sema);
  } else {
    // Unlike sem_timedwait on posix, semaphore_timedwait takes the time to
    // wait, not the deadline, so constructing the timespec is simpler here.
    NativeTime time = NativeTime::zero() + timeout;
    result = semaphore_timedwait(sema, time.to_platform());
  }
  if (result == KERN_SUCCESS)
    return true;
  if (result != KERN_OPERATION_TIMED_OUT)
    WARN("Waiting for semaphore failed: %i", result);
  return false;
}

bool NativeSemaphore::release() {
  kern_return_t result = semaphore_signal(sema);
  if (result == KERN_SUCCESS)
    return true;
  WARN("Call to semaphore_signal failed: %i", result);
  return false;
}
