//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <io.h>
#include "c/winhdr.h"

BEGIN_C_INCLUDES
#include "utils/string-inl.h"
END_C_INCLUDES

naked_file_handle_t AbstractStream::kNullNakedFileHandle = reinterpret_cast<naked_file_handle_t>(-1);

bool AbstractStream::is_a_tty() {
  naked_file_handle_t handle = to_raw_handle();
  if (handle == kNullNakedFileHandle)
    return false;
  int fd = _open_osfhandle(reinterpret_cast<intptr_t>(handle), 0);
  // TODO: does this leak file handles? The thing is, if we _close the fd then
  //   the underlying handle also gets closed which definitely isn't what we
  //   want.
  return (fd != -1) && _isatty(fd);
}

// A combined input- and output-stream that operates through a windows handle.
class HandleStream : public InOutStream {
public:
  explicit HandleStream(handle_t handle);
  virtual ~HandleStream();
  virtual void default_destroy() { default_delete_concrete(this); }
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

pass_def_ref_t<InOutStream> InOutStream::from_raw_handle(naked_file_handle_t handle) {
  return new (kDefaultAlloc) HandleStream(handle);
}

utf8_t FileSystem::get_temporary_file_name(utf8_t unique, char *dest, size_t dest_size) {
  CHECK_REL("buffer too small", dest_size, >=, MAX_PATH);
  char temp_path_buf[MAX_PATH];
  if (GetTempPath(MAX_PATH, temp_path_buf) == 0) {
    LOG_ERROR("GetTempPath(%i, -): %i", MAX_PATH, GetLastError());
    return string_empty();
  }
  if (GetTempFileName(temp_path_buf, unique.chars, 0, dest) == 0) {
    LOG_ERROR("GetTempFileName(%s, %s, 0, -): %i", temp_path_buf, unique.chars,
        GetLastError());
    return string_empty();
  }
  return new_c_string(dest);
}
