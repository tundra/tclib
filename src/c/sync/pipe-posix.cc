//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

#include "io/file.hh"

BEGIN_C_INCLUDES
#include "utils/string-inl.h"
END_C_INCLUDES

using namespace tclib;

bool NativePipe::open(uint32_t flags) {
  errno = 0;
  int result = ::pipe(this->pipe_);
  if (result == 0) {
    in_ = *InOutStream::from_raw_handle(this->pipe_[0]);
    out_ = *InOutStream::from_raw_handle(this->pipe_[1]);
    return true;
  }
  WARN("Call to pipe failed: %i (error: %s)", errno, strerror(errno));
  return false;
}

class PosixServerChannel : public ServerChannel {
public:
  PosixServerChannel();
  virtual ~PosixServerChannel();
  virtual void default_destroy() { default_delete_concrete(this); }
  virtual bool create(uint32_t flags);
  virtual bool open();
  virtual bool close();
  virtual utf8_t name() { return basename_; }
  virtual InStream *in() { return in_; }
  virtual OutStream *out() { return out_; }
  static ServerChannel *create();

  static bool connect_fifo(utf8_t basename, const char *suffix, int32_t mode,
      FileStreams *io_out);

private:
  static utf8_t get_fifo_name(utf8_t basename, const char *suffix,
      char *scratch, size_t bufsize);

  // Creates a fifo but does not connect to it.
  bool make_fifo(const char *suffix);

  #define kUpSuffix ".up"
  #define kDownSuffix ".down"

  utf8_t basename_;
  InStream *in_;
  OutStream *out_;
};

PosixServerChannel::PosixServerChannel()
  : basename_(string_empty())
  , in_(NULL)
  , out_(NULL) { }

PosixServerChannel::~PosixServerChannel() {
  string_default_delete(basename_);
  delete in_;
  delete out_;
}

utf8_t PosixServerChannel::get_fifo_name(utf8_t basename, const char *suffix,
    char *scratch,
    size_t bufsize) {
  CHECK_REL("scratch too small", bufsize, >=, 1024);
  int len = sprintf(scratch, "%s%s", basename.chars, suffix);
  return new_string(scratch, len);
}

bool PosixServerChannel::make_fifo(const char *suffix) {
  char scratch[1024];
  utf8_t name = get_fifo_name(basename_, suffix, scratch, 1024);
  errno = 0;
  int result = ::mkfifo(name.chars, 0600);
  if (result != 0) {
    WARN("mkfifo(%s, -) failed: %i (error: %s)", name.chars, errno, strerror(errno));
    return false;
  }
  return true;
}

bool PosixServerChannel::create(uint32_t flags) {
  char scratch[1024];
  utf8_t temp_name = FileSystem::get_temporary_file_name(new_c_string("srvchn"),
      scratch, 1024);
  basename_ = string_default_dup(temp_name);
  // First create the two fifos; we can't connect before both have been created
  // because connecting may block.
  return make_fifo(kUpSuffix) && make_fifo(kDownSuffix);
}

bool PosixServerChannel::connect_fifo(utf8_t basename, const char *suffix, int32_t mode,
    FileStreams *io_out) {
  char scratch[1024];
  utf8_t name = get_fifo_name(basename, suffix, scratch, 1024);
  *io_out = FileSystem::native()->open(name, mode);
  return true;
}

bool PosixServerChannel::open() {
  FileStreams up;
  FileStreams down;
  if (!connect_fifo(basename_, kUpSuffix, OPEN_FILE_MODE_READ, &up)
      || !connect_fifo(basename_, kDownSuffix, OPEN_FILE_MODE_WRITE, &down))
    return false;
  in_ = up.in();
  out_ = down.out();
  return true;
}

bool PosixServerChannel::close() {
  if (!(in_ == NULL || in_->close()) || !(out_ == NULL || out_->close()))
    return false;
  char scratch[1024];
  utf8_t up_name = get_fifo_name(basename_, kUpSuffix, scratch, 1024);
  int unlink_up = unlink(up_name.chars);
  utf8_t down_name = get_fifo_name(basename_, kDownSuffix, scratch, 1024);
  int unlink_down = unlink(down_name.chars);
  return (unlink_up == 0) && (unlink_down == 0);
}

pass_def_ref_t<ServerChannel> ServerChannel::create() {
  return pass_def_ref_t<ServerChannel>(new (kDefaultAlloc) PosixServerChannel());
}

class PosixClientChannel : public ClientChannel {
public:
  PosixClientChannel();
  virtual ~PosixClientChannel();
  virtual bool open(utf8_t basename);
  virtual InStream *in() { return in_; }
  virtual OutStream *out() { return out_; }
  virtual void default_destroy() { default_delete_concrete(this); }

private:
  InStream *in_;
  OutStream *out_;
};

PosixClientChannel::PosixClientChannel()
  : in_(NULL)
  , out_(NULL) { }

PosixClientChannel::~PosixClientChannel() {
  delete in_;
  delete out_;
}

bool PosixClientChannel::open(utf8_t basename) {
  FileStreams up;
  FileStreams down;
  if (!PosixServerChannel::connect_fifo(basename, kUpSuffix, OPEN_FILE_MODE_WRITE, &up)
      || !PosixServerChannel::connect_fifo(basename, kDownSuffix, OPEN_FILE_MODE_READ, &down))
    return false;
  in_ = down.in();
  out_ = up.out();
  return true;
}

pass_def_ref_t<ClientChannel> ClientChannel::create() {
  return pass_def_ref_t<ClientChannel>(new (kDefaultAlloc) PosixClientChannel());
}
