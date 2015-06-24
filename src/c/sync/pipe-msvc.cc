//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

using namespace tclib;

// A combined input- and output-stream that operates through a windows handle.
// This could be useful in multiple places so eventually it might be moved to
// its own file. Also, possibly it should be split into a separate in and out
// type.
class HandleStream : public InStream, public OutStream {
public:
  explicit HandleStream(handle_t handle);
  virtual ~HandleStream();
  virtual bool read_sync(read_iop_state_t *op);
  virtual bool write_sync(write_iop_state_t *op);
  virtual bool flush();
  virtual bool close();
  virtual naked_file_handle_t to_raw_handle();

private:
  bool is_closed_;
  handle_t handle_;
  OVERLAPPED overlapped_;
};

HandleStream::HandleStream(handle_t handle)
  : is_closed_(false)
  , handle_(handle) {
  ZeroMemory(&overlapped_, sizeof(overlapped_));
}

HandleStream::~HandleStream() {
  close();
}

bool HandleStream::read_sync(read_iop_state_t *op) {
  dword_t bytes_read = 0;
  // Because this handle can be read both sync and async we make it overlapped,
  // and then we have to always provide an overlapped struct when reading even
  // when it's a sync read.
  bool result = ReadFile(
      handle_,                              // hFile
      op->dest_,                            // lpBuffer
      static_cast<dword_t>(op->dest_size_), // nNumberOfBytesToRead
      &bytes_read,                          // lpNumberOfBytesRead
      &overlapped_);                        // lpOverlapped
  bool at_eof = false;
  if (!result) {
    if (GetLastError() == ERROR_IO_PENDING) {
      result = GetOverlappedResult(
          handle_,      // hFile
          &overlapped_, // lpOverlapped
          &bytes_read,  // lpNumberOfBytesTransferred
          true);        // bWait
    } else {
      at_eof = true;
    }
  }
  at_eof = at_eof || (!result && (bytes_read == 0));
  read_iop_state_deliver(op, bytes_read, at_eof);
  return true;
}

bool HandleStream::write_sync(write_iop_state_t *op) {
  dword_t bytes_written = 0;
  bool result = WriteFile(
    handle_,                             // hFile
    op->src,                             // lpBuffer
    static_cast<dword_t>(op->src_size),  // nNumberOfBytesToWrite
    &bytes_written,                      // lpNumberOfBytesWritten
    &overlapped_);                       // lpOverlapped
  if (!result) {
    if (GetLastError() == ERROR_IO_PENDING) {
      result = GetOverlappedResult(
          handle_,        // hFile
          &overlapped_,   // lpOverlapped
          &bytes_written, // lpNumberOfBytesTransferred
          true);          // bWait
    }
  }
  write_iop_state_deliver(op, bytes_written);
  return result;
}

bool HandleStream::flush() {
  return true;
}

bool HandleStream::close() {
  if (is_closed_)
    return true;
  is_closed_ = true;
  return CloseHandle(handle_);
}

naked_file_handle_t HandleStream::to_raw_handle() {
  return handle_;
}

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
    in_ = new HandleStream(this->pipe_.read_);
    out_ = new HandleStream(this->pipe_.write_);
  }
  return result;
}
