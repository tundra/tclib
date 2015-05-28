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
  virtual bool read_sync(read_iop_t *op);
  virtual bool at_eof();
  virtual size_t write_bytes(const void *src, size_t size);
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

bool FdStream::read_sync(read_iop_t *op) {
  ssize_t count = read(fd_, op->dest_, op->dest_size_);
  if (count == 0) {
    int e = errno;
    op->at_eof_ = !((e == EINTR) || (e == EAGAIN));
  }
  op->read_out_ = count;
  return true;
}

bool FdStream::at_eof() {
  return true;
}

size_t FdStream::write_bytes(const void *src, size_t size) {
  return write(fd_, src, size);
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
  int result = ::pipe(this->pipe);
  if (result == 0) {
    in_ = new FdStream(this->pipe[0]);
    out_ = new FdStream(this->pipe[1]);
    return true;
  }
  WARN("Call to pipe failed: %i (error: %s)", errno, strerror(errno));
  return false;
}
