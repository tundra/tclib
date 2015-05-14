//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

// A combined input- and output-stream that operates through a windows handle.
// This could be useful in multiple places so eventually it might be moved to
// its own file. Also, possibly it should be split into a separate in and out
// type.
class HandleStream : public InStream, public OutStream {
public:
  explicit HandleStream(void *handle) : at_eof_(false), is_closed_(false), handle_(handle) { }
  virtual ~HandleStream();
  virtual size_t read_bytes(void *dest, size_t size);
  virtual bool at_eof();
  virtual size_t write_bytes(const void *src, size_t size);
  virtual bool flush();
  virtual bool close();

private:
  bool at_eof_;
  bool is_closed_;
  void *handle_;
};

HandleStream::~HandleStream() {
  close();
}

size_t HandleStream::read_bytes(void *dest, size_t size) {
  dword_t read = 0;
  bool result = ReadFile(
      handle_,                    // hFile
      dest,                       // lpBuffer
      static_cast<dword_t>(size), // nNumberOfBytesToRead
      &read,                      // lpNumberOfBytesRead
      NULL);                      // lpOverlapped
  if (!result && (read == 0))
    at_eof_ = true;
  return read;
}

bool HandleStream::at_eof() {
  return at_eof_;
}

size_t HandleStream::write_bytes(const void *src, size_t size) {
  dword_t written = 0;
  WriteFile(
    handle_,                    // hFile
    src,                        // lpBuffer
    static_cast<dword_t>(size), // nNumberOfBytesToWrite
    &written,                   // lpNumberOfBytesWritten
    NULL);                      // lpOverlapped
  return written;
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

bool NativePipe::open() {
  bool result = CreatePipe(
      &this->pipe.read_,  // hReadPipe
      &this->pipe.write_, // hWritePipe,
      NULL,               // lpPipeAttributes,
      0);                 // nSize
  if (result) {
    in_ = new HandleStream(this->pipe.read_);
    out_ = new HandleStream(this->pipe.write_);
  }
  return result;
}
