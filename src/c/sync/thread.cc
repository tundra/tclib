//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "thread.hh"

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
