//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <pthread.h>

class NativeThread::Data {
public:
  Data() : thread_(0) { }

  bool start(NativeThread *thread);

  void *join();

  static void *entry_point(void *arg);
private:
  pthread_t thread_;
};

void *NativeThread::Data::entry_point(void *arg) {
  NativeThread *thread = static_cast<NativeThread*>(arg);
  return thread->callback_();
}

bool NativeThread::Data::start(NativeThread *thread) {
  return pthread_create(&thread_, NULL, entry_point, thread) == 0;
}

void *NativeThread::Data::join() {
  void *value = NULL;
  pthread_join(thread_, &value);
  return value;
}