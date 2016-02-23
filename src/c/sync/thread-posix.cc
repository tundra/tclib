//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <errno.h>

#include "thread.hh"
#include "utils/clock.hh"
using namespace tclib;

void *NativeThread::entry_point(void *arg) {
  NativeThread *thread = static_cast<NativeThread*>(arg);
  CHECK_EQ("thread interaction out of order", thread->state_, tsStarted);
  return (thread->callback_)();
}

bool NativeThread::platform_start() {
  int result = pthread_create(&thread_, NULL, entry_point, this);
  if (result == 0)
    return true;
  WARN("Call to pthread_create failed: %i (error: %s)", result, strerror(result));
  return false;
}

bool NativeThread::platform_dispose() {
  return true;
}

void *NativeThread::join() {
  CHECK_EQ("thread interaction out of order", state_, tsStarted);
  void *value = NULL;
  int result = pthread_join(thread_, &value);
  if (result != 0) {
    WARN("Call to pthread_join failed: %i (error: %s)", result, strerror(result));
    return NULL;
  }
  state_ = tsJoined;
  return value;
}

native_thread_id_t NativeThread::get_current_id() {
  // Always succeeds according to the man page.
  return pthread_self();
}

bool NativeThread::yield() {
  return IF_MACH(sched_yield(), pthread_yield()) == 0;
}

bool NativeThread::ids_equal(native_thread_id_t a, native_thread_id_t b) {
  return pthread_equal(a, b) != 0;
}

bool NativeThread::sleep(Duration duration) {
  NativeTime native_duration = NativeTime::zero() + duration;
  return nanosleep(&native_duration.to_platform(), NULL) == 0;
}
