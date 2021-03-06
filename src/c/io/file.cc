//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "io/file.hh"

BEGIN_C_INCLUDES
#include "utils/strbuf.h"
END_C_INCLUDES

#include <stdio.h>
#include <stdarg.h>

struct file_handle_t { };

using namespace tclib;

namespace tclib {

// Handle for an open file. This is for bookkeeping more than anything.
class FileHandle : public InStream, public OutStream, public file_handle_t {
public:
  virtual ~FileHandle() { }
};

}

FileStreams::FileStreams() {
  file_streams_t::is_open = false;
  file_streams_t::file = NULL;
  file_streams_t::in = NULL;
  file_streams_t::out = NULL;
}

FileStreams::FileStreams(FileHandle *file, InStream *in, OutStream *out) {
  file_streams_t::is_open = (file != NULL);
  file_streams_t::file = file;
  file_streams_t::in = in;
  file_streams_t::out = out;
}

InStream *FileStreams::in() {
  return static_cast<InStream*>(file_streams_t::in);
}

OutStream *FileStreams::out() {
  return static_cast<OutStream*>(file_streams_t::out);
}

// File opened through stdio.
class StdioOpenFile : public FileHandle {
public:
  explicit StdioOpenFile(FILE *file)
    : file_(file)
    , is_closed_(false) {
    CHECK_FALSE("file must be non-null", file == NULL);
  }
  virtual ~StdioOpenFile();
  virtual void default_destroy() { default_delete_concrete(this); }
  virtual bool read_sync(read_iop_state_t *op);
  virtual bool write_sync(write_iop_state_t *op);
  virtual bool flush();
  virtual bool close();
  virtual naked_file_handle_t to_raw_handle();

private:
  FILE *file_;
  bool is_closed_;
};

StdioOpenFile::~StdioOpenFile() {
  close();
}

bool StdioOpenFile::close() {
  if (is_closed_)
    return true;
  ::fclose(file_);
  file_ = NULL;
  is_closed_ = true;
  return true;
}

bool StdioOpenFile::read_sync(read_iop_state_t *op) {
  CHECK_FALSE("file is closed", is_closed_);
  size_t bytes_read = fread(op->dest_, 1, op->dest_size_, file_);
  bool at_eof = (bytes_read == 0) && feof(file_);
  read_iop_state_deliver(op, bytes_read, at_eof);
  return true;
}

bool StdioOpenFile::write_sync(write_iop_state_t *op) {
  CHECK_FALSE("file is closed", is_closed_);
  size_t bytes_written = fwrite(op->src, 1, op->src_size, file_);
  write_iop_state_deliver(op, bytes_written);
  return true;
}

bool StdioOpenFile::flush() {
  CHECK_FALSE("file is closed", is_closed_);
  return fflush(file_) == 0;
}

// File system implementation that uses the standard stdio functions.
class StdioFileSystem : public FileSystem {
public:
  StdioFileSystem();
  virtual FileStreams open(utf8_t path, int32_t mode);
  virtual InStream *std_in() { return &stdin_; }
  virtual OutStream *std_out() { return &stdout_; }
  virtual OutStream *std_err() { return &stderr_; }
private:
  StdioOpenFile stdin_;
  StdioOpenFile stdout_;
  StdioOpenFile stderr_;
};

StdioFileSystem::StdioFileSystem()
  : stdin_(stdin)
  , stdout_(stdout)
  , stderr_(stderr) { }

FileStreams StdioFileSystem::open(utf8_t path, int32_t mode) {
  const char *mode_str = NULL;
  if ((mode & OPEN_FILE_MODE_READ) != 0) {
    // If this is just "r" windows will sometimes think files end before they
    // actually do for some reason. With "rb" it works correctly.
    mode_str = "rb";
  } else if ((mode & OPEN_FILE_MODE_WRITE) != 0) {
    mode_str = ((mode & OPEN_FILE_FLAG_BINARY) == 0) ? "w" : "wb";
  } else if ((mode & OPEN_FILE_MODE_READ_WRITE) != 0) {
    mode_str = "r+";
  }
  FILE *file = ::fopen(path.chars, mode_str);
  if (file == NULL) {
    return FileStreams(NULL, NULL, NULL);
  } else {
    FileHandle *handle = new StdioOpenFile(file);
    return FileStreams(handle,
        (mode & OPEN_FILE_MODE_READ) ? handle : NULL,
        (mode & OPEN_FILE_MODE_WRITE) ? handle : NULL);
  }
}

FileSystem *FileSystem::native() {
  static StdioFileSystem *instance = NULL;
  if (instance == NULL)
    instance = new StdioFileSystem();
  return instance;
}

file_system_t *file_system_native() {
  return FileSystem::native();
}

file_streams_t file_system_open(file_system_t *fs, utf8_t path,
    open_file_mode_t mode) {
  return static_cast<FileSystem*>(fs)->open(path, mode);
}

void FileStreams::close() {
  if (!is_open)
    return;
  delete static_cast<FileHandle*>(file);
  file_streams_t::is_open = false;
  file_streams_t::file = NULL;
  file_streams_t::in = NULL;
  file_streams_t::out = NULL;
}

void file_streams_close(file_streams_t *streams) {
  static_cast<FileStreams*>(streams)->close();
}

in_stream_t *file_system_stdin(file_system_t *fs) {
  return static_cast<FileSystem*>(fs)->std_in();
}

out_stream_t *file_system_stdout(file_system_t *fs) {
  return static_cast<FileSystem*>(fs)->std_out();
}

out_stream_t *file_system_stderr(file_system_t *fs) {
  return static_cast<FileSystem*>(fs)->std_err();
}

#ifdef IS_MSVC
#include <io.h>
#endif

naked_file_handle_t StdioOpenFile::to_raw_handle() {
  int fn = fileno(file_);
  return IF_MSVC(
      reinterpret_cast<naked_file_handle_t>(_get_osfhandle(fn)),
      fn);
}
