//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "thread.hh"

BEGIN_C_INCLUDES
#include "thread.h"
END_C_INCLUDES

using namespace tclib;

#ifdef IS_GCC
#include "thread-posix.cc"
#endif

#ifdef IS_MSVC
#include "thread-msvc.cc"
#endif

NativeThread::NativeThread(run_callback_t callback)
  : data_(new Data(callback)) { }

NativeThread::~NativeThread() {
  delete data();
}

extern "C" native_thread_t *new_native_thread(void *(callback)(void*), void *data) {
  NativeThread *result = new NativeThread(callback_t<void*(void)>(callback, data));
  return reinterpret_cast<native_thread_t*>(result);
}

extern "C" void dispose_native_thread(native_thread_t *thread) {
  delete reinterpret_cast<NativeThread*>(thread);
}

extern "C" bool native_thread_start(native_thread_t *thread) {
  return reinterpret_cast<NativeThread*>(thread)->start();
}

extern "C" void *native_thread_join(native_thread_t *thread) {
  return reinterpret_cast<NativeThread*>(thread)->join();
}
