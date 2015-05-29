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
  explicit StdioOpenFile(FILE *file) : file_(file) { }
  virtual ~StdioOpenFile();
  virtual bool read_sync(read_iop_t *op);
  virtual size_t write_bytes(const void *src, size_t size);
  virtual bool flush();
  virtual bool close();

private:
  FILE *file_;
};

StdioOpenFile::~StdioOpenFile() {
  close();
}

bool StdioOpenFile::close() {
  if (file_ != NULL)
    ::fclose(file_);
  file_ = NULL;
  return true;
}

bool StdioOpenFile::read_sync(read_iop_t *op) {
  size_t bytes_read = fread(op->dest_, 1, op->dest_size_, file_);
  bool at_eof = (bytes_read == 0) && (feof(file_) != 0);
  read_iop_deliver(op, bytes_read, at_eof);
  return true;
}

size_t StdioOpenFile::write_bytes(const void *src, size_t size) {
  return fwrite(src, 1, size, file_);
}

bool StdioOpenFile::flush() {
  return fflush(file_) == 0;
}
// File system implementation that uses the standard stdio functions.
class StdioFileSystem : public FileSystem {
public:
  StdioFileSystem();
  virtual FileStreams open(utf8_t path, open_file_mode_t mode);
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

FileStreams StdioFileSystem::open(utf8_t path, open_file_mode_t mode) {
  const char *mode_str = NULL;
  switch (mode) {
    case OPEN_FILE_MODE_READ:
      // If this is just "r" windows will sometimes think files end before they
      // actually do for some reason. With "rb" it works correctly.
      mode_str = "rb";
      break;
    case OPEN_FILE_MODE_WRITE:
      mode_str = "w";
      break;
    case OPEN_FILE_MODE_READ_WRITE:
      mode_str = "r+";
      break;
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
