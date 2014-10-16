//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "file.hh"

#include <stdio.h>
#include <stdarg.h>

using namespace tclib;

size_t OpenFile::printf(const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  size_t result = vprintf(fmt, argp);
  va_end(argp);
  return result;
}

// File opened through stdio.
class StdioOpenFile : public OpenFile {
public:
  explicit StdioOpenFile(FILE *file) : file_(file) { }
  virtual ~StdioOpenFile();
  virtual size_t read_bytes(void *dest, size_t size);
  virtual bool at_eof();
  virtual size_t vprintf(const char *fmt, va_list argp);
  virtual bool flush();
private:
  FILE *file_;
};

StdioOpenFile::~StdioOpenFile() {
  ::fclose(file_);
  file_ = NULL;
}

size_t StdioOpenFile::read_bytes(void *dest, size_t size) {
  return fread(dest, 1, size, file_);
}

bool StdioOpenFile::at_eof() {
  return feof(file_) != 0;
}

size_t StdioOpenFile::vprintf(const char *fmt, va_list argp) {
  va_list next;
  va_copy(next, argp);
  size_t result = ::vfprintf(file_, fmt, next);
  va_end(next);
  return result;
}

bool StdioOpenFile::flush() {
  return fflush(file_) == 0;
}

void open_file_close(open_file_t *file) {
  delete static_cast<OpenFile*>(file);
}

bool open_file_flush(open_file_t *file) {
  return static_cast<OpenFile*>(file)->flush();
}

size_t open_file_read_bytes(open_file_t *file, void *dest, size_t size) {
  return static_cast<OpenFile*>(file)->read_bytes(dest, size);
}

bool open_file_at_eof(open_file_t *file) {
  return static_cast<OpenFile*>(file)->at_eof();
}

size_t open_file_vprintf(open_file_t *file, const char *fmt, va_list argp) {
  va_list next;
  va_copy(next, argp);
  size_t result = static_cast<OpenFile*>(file)->vprintf(fmt, next);
  va_end(next);
  return result;
}

size_t open_file_printf(open_file_t *file, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  size_t result = static_cast<OpenFile*>(file)->vprintf(fmt, argp);
  va_end(argp);
  return result;
}

// File system implementation that uses the standard stdio functions.
class StdioFileSystem : public FileSystem {
public:
  StdioFileSystem();
  virtual StdioOpenFile *open(const char *path, open_file_mode_t mode);
  virtual StdioOpenFile *std_in() { return &stdin_; }
  virtual StdioOpenFile *std_out() { return &stdout_; }
  virtual StdioOpenFile *std_err() { return &stderr_; }
private:
  StdioOpenFile stdin_;
  StdioOpenFile stdout_;
  StdioOpenFile stderr_;
};

StdioFileSystem::StdioFileSystem()
  : stdin_(stdin)
  , stdout_(stdout)
  , stderr_(stderr) { }

StdioOpenFile *StdioFileSystem::open(const char *path, open_file_mode_t mode) {
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
  FILE *file = ::fopen(path, mode_str);
  return (file == NULL) ? NULL : new StdioOpenFile(file);
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

open_file_t *file_system_open(file_system_t *fs, const char *path,
    open_file_mode_t mode) {
  return static_cast<FileSystem*>(fs)->open(path, mode);
}

open_file_t *file_system_stdin(file_system_t *fs) {
  return static_cast<FileSystem*>(fs)->std_in();
}

open_file_t *file_system_stdout(file_system_t *fs) {
  return static_cast<FileSystem*>(fs)->std_out();
}

open_file_t *file_system_stderr(file_system_t *fs) {
  return static_cast<FileSystem*>(fs)->std_err();
}
