//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "winhdr.h"

class NativeThread::Data {
public:
  Data()
    : thread_(INVALID_HANDLE_VALUE)
    , thread_id_(0)
    , result_(NULL) { }
  ~Data();
  bool start(NativeThread *thread);
  void *join();
  static dword_t __stdcall entry_point(void *arg);

private:
  handle_t thread_;
  dword_t thread_id_;
  void *result_;
};

NativeThread::Data::~Data() {
  if (thread_ != INVALID_HANDLE_VALUE) {
    if (!CloseHandle(thread_))
      WARN("Call to CloseHandle failed: %i", GetLastError());
  }
}

dword_t __stdcall NativeThread::Data::entry_point(void *arg) {
  NativeThread *thread = static_cast<NativeThread*>(arg);
  thread->data_->result_ = (thread->callback_)();
  return 0;
}

bool NativeThread::Data::start(NativeThread *thread) {
  handle_t result = CreateThread(
      NULL,         // lpThreadAttributes
      0,            // dwStackSize
      entry_point,  // lpStartAddress
      thread,       // lpParameter
      0,            // dwCreationFlags
      &thread_id_); // lpThreadId
  if (result == NULL) {
    WARN("Call to CreateThread failed: %i", GetLastError());
    return false;
  }
  thread_ = result;
  return true;
}

void *NativeThread::Data::join() {
  if (WaitForSingleObject(thread_, INFINITE) != WAIT_OBJECT_0)
    WARN("Call to WaitForSingleObject failed: %i", GetLastError());
  return result_;
}

native_thread_id_t NativeThread::get_current_id() {
  return GetCurrentThreadId();
}

bool NativeThread::ids_equal(native_thread_id_t a, native_thread_id_t b) {
  return a == b;
}
