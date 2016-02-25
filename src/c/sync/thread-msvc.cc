//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

#include "sync/thread.hh"
using namespace tclib;

fat_bool_t NativeThread::platform_dispose() {
  if (thread_ != INVALID_HANDLE_VALUE) {
    if (!CloseHandle(thread_)) {
      WARN("Call to CloseHandle failed: %i", GetLastError());
      return F_FALSE;
    }
  }
  return F_TRUE;
}

unsigned long __stdcall NativeThread::entry_point(void *arg) {
  NativeThread *thread = static_cast<NativeThread*>(arg);
  CHECK_EQ("thread interaction out of order", thread->state_, tsStarted);
  thread->result_ = (thread->callback_)();
  return 0;
}

fat_bool_t NativeThread::platform_start() {
  handle_t result = CreateThread(
      NULL,         // lpThreadAttributes
      0,            // dwStackSize
      entry_point,  // lpStartAddress
      this,         // lpParameter
      0,            // dwCreationFlags
      NULL);        // lpThreadId
  if (result == NULL) {
    WARN("Call to CreateThread failed: %i", GetLastError());
    return F_FALSE;
  }
  thread_ = result;
  return F_TRUE;
}

fat_bool_t NativeThread::join(opaque_t *value_out) {
  CHECK_EQ("thread interaction out of order", state_, tsStarted);
  if (WaitForSingleObject(thread_, INFINITE) != WAIT_OBJECT_0) {
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
    return F_FALSE;
  }
  state_ = tsJoined;
  if (value_out != NULL)
    *value_out = result_;
  return F_TRUE;
}

native_thread_id_t NativeThread::get_current_id() {
  return GetCurrentThreadId();
}

bool NativeThread::yield() {
  return SwitchToThread();
}

bool NativeThread::ids_equal(native_thread_id_t a, native_thread_id_t b) {
  return a == b;
}

bool NativeThread::sleep(Duration duration) {
  Sleep(duration.to_winapi_millis());
  return true;
}
