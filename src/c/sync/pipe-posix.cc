//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <unistd.h>
#include <errno.h>

// A combined input- and output-stream that operates through a file descriptor.
// This could be useful in multiple places so eventually it might be moved to
// its own file. Also, possibly it should be split into a separate in and out
// type.
class FdStream : public InStream, public OutStream {
public:
  explicit FdStream(int fd) : at_eof_(false), is_closed_(false), fd_(fd) { }
  virtual ~FdStream();
  virtual size_t read_bytes(void *dest, size_t size);
  virtual bool at_eof();
  virtual size_t write_bytes(const void *src, size_t size);
  virtual bool flush();
  virtual bool close();

private:
  bool at_eof_;
  bool is_closed_;
  int fd_;
};

FdStream::~FdStream() {
  close();
}

size_t FdStream::read_bytes(void *dest, size_t size) {
  ssize_t count = read(fd_, dest, size);
  if (count == 0) {
    int e = errno;
    at_eof_ = !((e == EINTR) || (e == EAGAIN));
    return 0;
  } else {
    return count;
  }
}

bool FdStream::at_eof() {
  return at_eof_;
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

bool NativePipe::open() {
  int result = ::pipe(this->pipe);
  if (result == 0) {
    in_ = new FdStream(this->pipe[0]);
    out_ = new FdStream(this->pipe[1]);
    return true;
  }
  WARN("Call to pipe failed: %i (error: %s)", errno, strerror(errno));
  return false;
}

