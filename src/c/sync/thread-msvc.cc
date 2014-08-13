//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#pragma warning(push, 0)
#include <windows.h>
#pragma warning(pop)

class NativeThread::Data {
public:
  Data(run_callback_t callback)
    : callback_(callback)
    , thread_(INVALID_HANDLE_VALUE)
    , thread_id_(0)
    , result_(NULL) { }
  ~Data();
  HANDLE &thread() { return thread_; }
  DWORD &thread_id() { return thread_id_; }
  void *result() { return result_; }
  void run();


  static DWORD __stdcall run_bridge(LPVOID arg);
private:
  run_callback_t callback_;
  HANDLE thread_;
  DWORD thread_id_;
  void *result_;
};

NativeThread::Data::~Data() {
  if (thread_ != INVALID_HANDLE_VALUE)
    CloseHandle(thread_);
}

void NativeThread::Data::run() {
  result_ = callback_();
}

DWORD __stdcall NativeThread::Data::run_bridge(LPVOID arg) {
  Data *data = static_cast<Data*>(arg);
  data->run();
  return 0;
}

bool NativeThread::start() {
  data()->thread() = CreateThread(
      NULL,                  // lpThreadAttributes
      0,                     // dwStackSize
      &Data::run_bridge,      // lpStartAddress
      data(),                // lpParameter
      0,                     // dwCreationFlags
      &data()->thread_id()); // lpThreadId
  return true;
}

void *NativeThread::join() {
  WaitForSingleObject(data()->thread(), INFINITE);
  return data()->result();
}
