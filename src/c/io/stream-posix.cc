//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <errno.h>
#include <unistd.h>

// It's a coincidence that the convention on both platforms happens to be -1.
naked_file_handle_t AbstractStream::kNullNakedFileHandle = -1;

bool AbstractStream::is_a_tty() {
  naked_file_handle_t handle = to_raw_handle();
  if (handle == kNullNakedFileHandle)
    return false;
  return isatty(handle);
}

// A combined input- and output-stream that operates through a file descriptor.
class FdStream : public InOutStream {
public:
  explicit FdStream(int fd) : is_closed_(false), fd_(fd) { }
  virtual ~FdStream();
  virtual bool read_sync(read_iop_state_t *op);
  virtual bool write_sync(write_iop_state_t *op);
  virtual bool flush();
  virtual bool close();
  virtual naked_file_handle_t to_raw_handle();

private:
  bool is_closed_;
  int fd_;
};

FdStream::~FdStream() {
  close();
}

bool FdStream::read_sync(read_iop_state_t *op) {
  ssize_t bytes_read = 0;
  do {
    errno = 0;
    bytes_read = read(fd_, op->dest_, op->dest_size_);
  } while (bytes_read == 0 && errno == EINTR);
  bool at_eof = (bytes_read == 0) && (errno != EAGAIN);
  read_iop_state_deliver(op, bytes_read, at_eof);
  return true;
}

bool FdStream::write_sync(write_iop_state_t *op) {
  size_t bytes_written = write(fd_, op->src, op->src_size);
  write_iop_state_deliver(op, bytes_written);
  return true;
}

bool FdStream::flush() {
  return fsync(fd_) == 0;
}

bool FdStream::close() {
  if (is_closed_)
    return true;
  is_closed_ = true;
  return ::close(fd_) == 0;
}

naked_file_handle_t FdStream::to_raw_handle() {
  return fd_;
}

InOutStream *InOutStream::from_raw_handle(naked_file_handle_t handle) {
  return new FdStream(handle);
}
