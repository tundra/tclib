//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <pthread.h>

class NativeThread::Data {
public:
  Data(run_callback_t callback)
    : callback_(callback)
    , thread_(0) { }
  pthread_t &thread() { return thread_; }

  static void *run_bridge(void *arg);
private:
  run_callback_t callback_;
  pthread_t thread_;
};

void *NativeThread::Data::run_bridge(void *arg) {
  Data *data = static_cast<Data*>(arg);
  return data->callback_();
}

void NativeThread::start() {
  pthread_create(&data()->thread(), NULL, Data::run_bridge, data());
}

void *NativeThread::join() {
  void *value = NULL;
  pthread_join(data()->thread(), &value);
  return value;
}
