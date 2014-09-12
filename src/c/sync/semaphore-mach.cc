//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Mach looks like it supports posix-style unnamed semaphores through the posix
// api but actually it doesn't, the calls fail.

#include <mach/semaphore.h>
#include <mach/mach.h>

class NativeSemaphore::Data {
public:
  ~Data();
  bool initialize(uint32_t initial_count);
  bool acquire();
  bool try_acquire();
  bool release();
private:
  semaphore_t sema_;
};

bool NativeSemaphore::Data::initialize(uint32_t initial_count) {
  kern_return_t result = semaphore_create(mach_task_self(), &sema_,
      SYNC_POLICY_FIFO, -1);
  if (result == KERN_SUCCESS)
    return true;
  WARN("Call to semaphore_create failed: %i", result);
  return false;
}

NativeSemaphore::Data::~Data() {
  kern_return_t result = semaphore_destroy(mach_task_self(), sema_);
  if (result != KERN_SUCCESS)
    WARN("Call to semaphore_destroy failed: %i", result);
}

bool NativeSemaphore::Data::acquire() {
  kern_return_t result = semaphore_wait(sema_);
  if (result == KERN_SUCCESS)
    return true;
  WARN("Call to semaphore_wait failed: %i", result);
  return false;
}

bool NativeSemaphore::Data::try_acquire() {
  mach_timespec time;
  time.tv_sec = 0;
  time.tv_nsec = 0;
  kern_return_t result = semaphore_timedwait(sema_, time);
  if (result == KERN_SUCCESS)
    return true;
  if (result != KERN_OPERATION_TIMED_OUT)
    // Not being able to acquire will look like a timeout so don't warn on that,
    // we expect it.
    WARN("Call to semaphore_timedwait failed: %i", result);
  return false;
}

bool NativeSemaphore::Data::release() {
  kern_return_t result = semaphore_signal(sema_);
  if (result == KERN_SUCCESS)
    return true;
  WARN("Call to semaphore_signal failed: %i", result);
  return false;
}
