//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

#include "sync/thread.hh"
using namespace tclib;

bool NativeThread::platform_dispose() {
  if (thread_.handle_ != INVALID_HANDLE_VALUE) {
    if (!CloseHandle(thread_.handle_)) {
      WARN("Call to CloseHandle failed: %i", GetLastError());
      return false;
    }
  }
  return true;
}

unsigned long __stdcall NativeThread::entry_point(void *arg) {
  NativeThread *thread = static_cast<NativeThread*>(arg);
  thread->thread_.result_ = (thread->callback_)();
  return 0;
}

bool NativeThread::platform_start() {
  handle_t result = CreateThread(
      NULL,         // lpThreadAttributes
      0,            // dwStackSize
      entry_point,  // lpStartAddress
      this,         // lpParameter
      0,            // dwCreationFlags
      NULL);        // lpThreadId
  if (result == NULL) {
    WARN("Call to CreateThread failed: %i", GetLastError());
    return false;
  }
  thread_.handle_ = result;
  return true;
}

void *NativeThread::join() {
  if (WaitForSingleObject(thread_.handle_, INFINITE) != WAIT_OBJECT_0)
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
  return thread_.result_;
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
