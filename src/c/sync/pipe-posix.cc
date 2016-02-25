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

fat_bool_t NativePipe::open(uint32_t flags) {
  errno = 0;
  int result = ::pipe(this->pipe_);
  if (result == 0) {
    in_ = *InOutStream::from_raw_handle(this->pipe_[0]);
    out_ = *InOutStream::from_raw_handle(this->pipe_[1]);
    return F_TRUE;
  }
  WARN("Call to pipe failed: %i (error: %s)", errno, strerror(errno));
  return F_FALSE;
}

class PosixServerChannel : public ServerChannel {
public:
  PosixServerChannel();
  virtual ~PosixServerChannel();
  virtual void default_destroy() { default_delete_concrete(this); }
  virtual fat_bool_t allocate(uint32_t flags);
  virtual fat_bool_t open();
  virtual fat_bool_t close();
  virtual utf8_t name() { return basename_; }
  virtual InStream *in() { return in_; }
  virtual OutStream *out() { return out_; }
  static ServerChannel *create();

  static fat_bool_t connect_fifo(utf8_t basename, const char *suffix, int32_t mode,
      FileStreams *io_out);

private:
  static utf8_t get_fifo_name(utf8_t basename, const char *suffix,
      char *scratch, size_t bufsize);

  // Creates a fifo but does not connect to it.
  fat_bool_t make_fifo(const char *suffix);

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

fat_bool_t PosixServerChannel::make_fifo(const char *suffix) {
  char scratch[1024];
  utf8_t name = get_fifo_name(basename_, suffix, scratch, 1024);
  errno = 0;
  int result = ::mkfifo(name.chars, 0600);
  if (result != 0) {
    WARN("mkfifo(%s, -) failed: %i (error: %s)", name.chars, errno, strerror(errno));
    return F_FALSE;
  }
  return F_TRUE;
}

fat_bool_t PosixServerChannel::allocate(uint32_t flags) {
  char scratch[1024];
  utf8_t temp_name = FileSystem::get_temporary_file_name(new_c_string("srvchn"),
      scratch, 1024);
  basename_ = string_default_dup(temp_name);
  // First create the two fifos; we can't connect before both have been created
  // because connecting may block.
  F_TRY(make_fifo(kUpSuffix));
  F_TRY(make_fifo(kDownSuffix));
  return F_TRUE;
}

fat_bool_t PosixServerChannel::connect_fifo(utf8_t basename, const char *suffix, int32_t mode,
    FileStreams *io_out) {
  char scratch[1024];
  utf8_t name = get_fifo_name(basename, suffix, scratch, 1024);
  *io_out = FileSystem::native()->open(name, mode);
  return F_TRUE;
}

fat_bool_t PosixServerChannel::open() {
  FileStreams up;
  FileStreams down;
  F_TRY(connect_fifo(basename_, kUpSuffix, OPEN_FILE_MODE_READ, &up));
  F_TRY(connect_fifo(basename_, kDownSuffix, OPEN_FILE_MODE_WRITE, &down));
  in_ = up.in();
  out_ = down.out();
  return F_TRUE;
}

fat_bool_t PosixServerChannel::close() {
  if (!(in_ == NULL || in_->close()) || !(out_ == NULL || out_->close()))
    return F_FALSE;
  char scratch[1024];
  utf8_t up_name = get_fifo_name(basename_, kUpSuffix, scratch, 1024);
  int unlink_up = unlink(up_name.chars);
  utf8_t down_name = get_fifo_name(basename_, kDownSuffix, scratch, 1024);
  int unlink_down = unlink(down_name.chars);
  if (unlink_up != 0)
    return F_FALSE;
  if (unlink_down != 0)
    return F_FALSE;
  return F_TRUE;
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
