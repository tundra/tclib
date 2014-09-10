//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

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

native_thread_id_t NativeThread::get_current_id() {
  return pthread_self();
}

bool NativeThread::ids_equal(native_thread_id_t a, native_thread_id_t b) {
  return pthread_equal(a, b) != 0;
}
