//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "io/file.hh"

using namespace tclib;

bool NativePipe::open(uint32_t flags) {
  if ((flags & pfGenerateName) != 0) {
    char buf[1024];
    utf8_t unique = new_c_string("nativepipe");
    utf8_t name = FileSystem::get_temporary_file_name(unique, buf, 1024);
    name_ = string_default_dup(name);
    errno = 0;
    int result = ::mkfifo(name.chars, 0600);
    if (result != 0) {
      WARN("Call to mkfifo failed: %i (error: %s)", errno, strerror(errno));
      return false;
    }
    FileStreams streams = FileSystem::native()->open(name, OPEN_FILE_MODE_READ_WRITE);
    in_ = streams.in();
    out_ = streams.out();
    in_is_out_ = streams.in_is_out();
  } else {
    errno = 0;
    int result = ::pipe(this->pipe_);
    if (result != 0) {
      WARN("Call to pipe failed: %i (error: %s)", errno, strerror(errno));
      return false;
    }
    in_ = InOutStream::from_raw_handle(this->pipe_[0]);
    out_ = InOutStream::from_raw_handle(this->pipe_[1]);
  }
  return true;
}

bool NativePipe::ensure_destroyed(utf8_t name) {
  errno = 0;
  int result = unlink(name.chars);
  if (result != 0) {
    ERROR("unlink(%s): %i", name.chars, errno);
    return false;
  }
  return true;
}
