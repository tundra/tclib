//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <unistd.h>
#include <errno.h>

using namespace tclib;

// A combined input- and output-stream that operates through a file descriptor.
// This could be useful in multiple places so eventually it might be moved to
// its own file. Also, possibly it should be split into a separate in and out
// type.
class FdStream : public InStream, public OutStream {
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

bool NativePipe::open(uint32_t flags) {
  errno = 0;
  int result = ::pipe(this->pipe_);
  if (result == 0) {
    in_ = new FdStream(this->pipe_[0]);
    out_ = new FdStream(this->pipe_[1]);
    return true;
  }
  WARN("Call to pipe failed: %i (error: %s)", errno, strerror(errno));
  return false;
}
