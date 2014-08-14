//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <pthread.h>

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
  return pthread_mutex_init(&mutex_, &attr) == 0;
}

NativeMutex::Data::~Data() {
  pthread_mutex_destroy(&mutex_);
}

bool NativeMutex::Data::lock() {
  return pthread_mutex_lock(&mutex_) == 0;
}

bool NativeMutex::Data::try_lock() {
  return pthread_mutex_trylock(&mutex_) == 0;
}

bool NativeMutex::Data::unlock() {
  return pthread_mutex_unlock(&mutex_) == 0;
}
