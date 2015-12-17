//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

using namespace tclib;

static bool create_overlapped_pipe(handle_t *read_pipe, handle_t *write_pipe,
    SECURITY_ATTRIBUTES *pipe_attributes, dword_t size) {
  static size_t next_pipe_serial = 0;
  char pipe_name[MAX_PATH];

  // Generate a unique name for this pipe. There is a race condition here in
  // that access to next_pipe_serial is unsynchronized but the serial only has
  // to be unique within one thread so if we include the thread id we should
  // be fine. I think we're fine. Pretty sure we're fine.
  sprintf(pipe_name, "\\\\.\\Pipe\\TclibPipe-%016x-%016x-%016x",
      GetCurrentProcessId(), GetCurrentThreadId(), next_pipe_serial++);

  if (size == 0)
    size = 4096;

  handle_t read = CreateNamedPipe(
      pipe_name,                                  // lpName
      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,  // dwOpenMode
      PIPE_TYPE_BYTE | PIPE_WAIT,                 // dwPipeMode
      1,                                          // nMaxInstances
      size,                                       // nOutBufferSize
      size,                                       // nInBufferSize
      120 * 1000,                                 // nDefaultTimeOut
      pipe_attributes);                           // lpSecurityAttributes

  if (read == INVALID_HANDLE_VALUE)
    return false;

  handle_t write = CreateFile(
      pipe_name,                                    // lpFileName
      GENERIC_WRITE,                                // dwDesiredAccess
      0,                                            // dwShareMode
      pipe_attributes,                              // lpSecurityAttributes
      OPEN_EXISTING,                                // dwCreationDisposition
      FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED, // dwFlagsAndAttributes
      NULL);                                        // hTemplateFile

  if (write == INVALID_HANDLE_VALUE)
    return false;

  *read_pipe = read;
  *write_pipe = write;
  return true;
}

bool NativePipe::open(uint32_t flags) {
  SECURITY_ATTRIBUTES attrs;
  ZeroMemory(&attrs, sizeof(attrs));
  attrs.nLength = sizeof(attrs);
  attrs.bInheritHandle = (flags & pfInherit) != 0;
  attrs.lpSecurityDescriptor = NULL;

  bool result = create_overlapped_pipe(
      &this->pipe_.read_,  // hReadPipe
      &this->pipe_.write_, // hWritePipe,
      &attrs,              // lpPipeAttributes,
      0);                  // nSize

  if (result) {
    in_ = InOutStream::from_raw_handle(this->pipe_.read_);
    out_ = InOutStream::from_raw_handle(this->pipe_.write_);
  }
  return result;
}
