//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "thread.hh"
#include <new>

BEGIN_C_INCLUDES
#include "thread.h"
#include "utils/log.h"
END_C_INCLUDES

using namespace tclib;

#ifdef IS_GCC
#include "thread-posix.cc"
#endif

#ifdef IS_MSVC
#include "thread-msvc.cc"
#endif

NativeThread::NativeThread(run_callback_t callback)
  : callback_(callback)
  , is_initialized_(false) {
  platform_thread_t init = kPlatformThreadInit;
  thread_ = init;
}

NativeThread::NativeThread()
  : is_initialized_(false) {
  platform_thread_t init = kPlatformThreadInit;
  thread_ = init;
}

NativeThread::~NativeThread() {
  if (!is_initialized_)
    return;
  is_initialized_ = false;
  platform_dispose();
}

bool NativeThread::start() {
  if (callback_.is_empty())
    return false;
  is_initialized_ = platform_start();
  return is_initialized_;
}

void NativeThread::set_callback(run_callback_t callback) {
  callback_ = callback;
}

native_thread_t *new_native_thread(void *(callback)(void*), void *data) {
  NativeThread *result = new NativeThread(new_callback(callback, data));
  return reinterpret_cast<native_thread_t*>(result);
}

void dispose_native_thread(native_thread_t *thread) {
  delete reinterpret_cast<NativeThread*>(thread);
}

bool native_thread_start(native_thread_t *thread) {
  return reinterpret_cast<NativeThread*>(thread)->start();
}

void *native_thread_join(native_thread_t *thread) {
  return reinterpret_cast<NativeThread*>(thread)->join();
}

native_thread_id_t native_thread_get_current_id() {
  return NativeThread::get_current_id();
}

bool native_thread_ids_equal(native_thread_id_t a, native_thread_id_t b) {
  return NativeThread::ids_equal(a, b);
}
