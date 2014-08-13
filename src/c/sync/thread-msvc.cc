//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "winhdr.h"

class NativeThread::Data {
public:
  Data(run_callback_t callback)
    : callback_(callback)
    , thread_(INVALID_HANDLE_VALUE)
    , thread_id_(0)
    , result_(NULL) { }
  ~Data();
  handle_t &thread() { return thread_; }
  dword_t &thread_id() { return thread_id_; }
  void *result() { return result_; }
  void run();
  static dword_t __stdcall run_bridge(void *arg);

private:
  run_callback_t callback_;
  handle_t thread_;
  dword_t thread_id_;
  void *result_;
};

NativeThread::Data::~Data() {
  if (thread_ != INVALID_HANDLE_VALUE)
    CloseHandle(thread_);
}

void NativeThread::Data::run() {
  result_ = callback_();
}

dword_t __stdcall NativeThread::Data::run_bridge(void *arg) {
  Data *data = static_cast<Data*>(arg);
  data->run();
  return 0;
}

bool NativeThread::start() {
  data()->thread() = CreateThread(
      NULL,                  // lpThreadAttributes
      0,                     // dwStackSize
      Data::run_bridge,      // lpStartAddress
      data(),                // lpParameter
      0,                     // dwCreationFlags
      &data()->thread_id()); // lpThreadId
  return true;
}

void *NativeThread::join() {
  WaitForSingleObject(data()->thread(), INFINITE);
  return data()->result();
}
