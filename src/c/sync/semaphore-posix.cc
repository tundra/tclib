//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <semaphore.h>
#include <errno.h>

// Posix semaphores are different from the other concurrency primitives in that
// they return error codes through errno instead of their result values which
// will always be -1 on errors. It's okay though, errno should be thread safe.

class NativeSemaphore::Data {
public:
  ~Data();
  bool initialize(uint32_t initial_count);
  bool acquire();
  bool try_acquire();
  bool release();
private:
  sem_t sema_;
};

bool NativeSemaphore::Data::initialize(uint32_t initial_count) {
  int result = sem_init(&sema_, false, initial_count);
  if (result == 0)
    return true;
  WARN("Call to sem_init failed: %i (error: %s)", result, strerror(errno));
  return false;
}

NativeSemaphore::Data::~Data() {
  int result = sem_destroy(&sema_);
  if (result == 0)
    return;
  WARN("Call to sem_destroy failed: %i (error: %s)", result, strerror(errno));
}

bool NativeSemaphore::Data::acquire() {
  int result = sem_wait(&sema_);
  if (result == 0)
    return true;
  WARN("Call to sem_wait failed: %i (error: %s)", result, strerror(errno));
  return false;
}

bool NativeSemaphore::Data::try_acquire() {
  int result = sem_trywait(&sema_);
  if (result == 0)
    return true;
  if (errno != EAGAIN)
    // EAGAIN indicates that nothing went wrong as such, the semaphore is just
    // zero.
    WARN("Call to sem_trywait failed: %i (error: %i %s)", result, strerror(errno));
  return false;
}

bool NativeSemaphore::Data::release() {
  int result = sem_post(&sema_);
  if (result == 0)
    return true;
  WARN("Call to sem_post failed: %i (error: %s)", result, strerror(errno));
  return false;
}
