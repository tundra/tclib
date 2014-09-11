//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <pthread.h>
#include <errno.h>

class NativeMutex::Data {
public:
  ~Data();
  bool initialize();
  bool lock();
  bool try_lock();
  bool unlock();
private:
  pthread_mutex_t mutex_;
};

bool NativeMutex::Data::initialize() {
  pthread_mutexattr_t attr;
  pthread_mutexattr_init(&attr);
  pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
  int result = pthread_mutex_init(&mutex_, &attr);
  if (result == 0)
    return true;
  WARN("Call to pthread_mutex_init failed: %i (error: %s)", result, strerror(result));
  return false;
}

NativeMutex::Data::~Data() {
  int result = pthread_mutex_destroy(&mutex_);
  if (result == 0)
    return;
  WARN("Call to pthread_mutex_destroy failed: %i (error: %s)", result, strerror(result));
  exit(0);
}

bool NativeMutex::Data::lock() {
  int result = pthread_mutex_lock(&mutex_);
  if (result == 0)
    return true;
  WARN("Call to pthread_mutex_lock failed: %i (error: %s)", result, strerror(result));
  return false;
}

bool NativeMutex::Data::try_lock() {
  int result = pthread_mutex_trylock(&mutex_);
  if (result == 0)
    return true;
  if (result != EBUSY)
    // Busy means that it's already held which is a normal result so don't warn
    // on that.
    WARN("Call to pthread_mutex_trylock failed: %i (error: %s)", result, strerror(result));
  return false;
}

bool NativeMutex::Data::unlock() {
  int result = pthread_mutex_unlock(&mutex_);
  if (result == 0)
    return true;
  WARN("Call to pthread_mutex_unlock failed: %i (error: %s)", result, strerror(result));
  return false;
}
