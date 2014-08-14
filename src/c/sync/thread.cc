//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "thread.hh"
#include <new>

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
  : callback_(callback)
  , data_(NULL) { }

NativeThread::NativeThread()
  : data_(NULL) { }

NativeThread::~NativeThread() {
  if (data_ != NULL) {
    data_->~Data();
    data_ = NULL;
  }
}

bool NativeThread::start() {
  if (callback_.is_empty())
    return false;
  if (kMaxDataSize < sizeof(Data))
    return false;
  data_ = new (data_memory_) Data();
  return data_->start(this);
}

void *NativeThread::join() {
  return data_->join();
}

void NativeThread::set_callback(run_callback_t callback) {
  callback_ = callback;
}

size_t NativeThread::get_data_size() {
  return sizeof(Data);
}

native_thread_t *new_native_thread(void *(callback)(void*), void *data) {
  NativeThread *result = new NativeThread(callback_t<void*(void)>(callback, data));
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
