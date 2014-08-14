//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <semaphore.h>

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
  return sem_init(&sema_, false, initial_count) == 0;
}

NativeSemaphore::Data::~Data() {
  sem_destroy(&sema_);
}

bool NativeSemaphore::Data::acquire() {
  return sem_wait(&sema_) == 0;
}

bool NativeSemaphore::Data::try_acquire() {
  return sem_trywait(&sema_) == 0;
}

bool NativeSemaphore::Data::release() {
  return sem_post(&sema_) == 0;
}
