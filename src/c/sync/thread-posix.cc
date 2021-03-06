//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <errno.h>

#include "thread.hh"
#include "utils/clock.hh"
using namespace tclib;

void *NativeThread::entry_point(void *arg) {
  NativeThread *thread = static_cast<NativeThread*>(arg);
  CHECK_EQ("thread interaction out of order", thread->state_, tsStarted);
  thread->result_ = (thread->callback_)();
  return NULL;
}

fat_bool_t NativeThread::platform_start() {
  int result = pthread_create(&thread_, NULL, entry_point, this);
  if (result == 0)
    return F_TRUE;
  WARN("Call to pthread_create failed: %i (error: %s)", result, strerror(result));
  return F_FALSE;
}

fat_bool_t NativeThread::platform_dispose() {
  return F_TRUE;
}

fat_bool_t NativeThread::join(opaque_t *value_out) {
  CHECK_EQ("thread interaction out of order", state_, tsStarted);
  int result = pthread_join(thread_, NULL);
  if (result != 0) {
    WARN("Call to pthread_join failed: %i (error: %s)", result, strerror(result));
    return F_FALSE;
  }
  state_ = tsJoined;
  if (value_out != NULL)
    *value_out = result_;
  return F_TRUE;
}

native_thread_id_t NativeThread::get_current_id() {
  // Always succeeds according to the man page.
  return pthread_self();
}

fat_bool_t NativeThread::yield() {
  return F_BOOL(IF_MACH(sched_yield(), pthread_yield()) == 0);
}

bool NativeThread::ids_equal(native_thread_id_t a, native_thread_id_t b) {
  return pthread_equal(a, b) != 0;
}

fat_bool_t NativeThread::sleep(Duration duration) {
  NativeTime native_duration = NativeTime::zero() + duration;
  return F_BOOL(nanosleep(&native_duration.to_platform(), NULL) == 0);
}
