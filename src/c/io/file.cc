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

bool AbstractStream::close() {
  return true;
}

naked_file_handle_t AbstractStream::kNullNakedFileHandle = IF_MSVC(NULL, -1);

naked_file_handle_t AbstractStream::to_raw_handle() {
  return kNullNakedFileHandle;
}

size_t OutStream::printf(const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  size_t result = vprintf(fmt, argp);
  va_end(argp);
  return result;
}

// File opened through stdio.
class StdioOpenFile : public FileHandle {
public:
  explicit StdioOpenFile(FILE *file) : file_(file) { }
  virtual ~StdioOpenFile();
  virtual size_t read_bytes(void *dest, size_t size);
  virtual size_t write_bytes(const void *src, size_t size);
  virtual bool at_eof();
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

size_t StdioOpenFile::read_bytes(void *dest, size_t size) {
  return fread(dest, 1, size, file_);
}

size_t StdioOpenFile::write_bytes(const void *src, size_t size) {
  return fwrite(src, 1, size, file_);
}

bool StdioOpenFile::at_eof() {
  return feof(file_) != 0;
}

size_t OutStream::vprintf(const char *fmt, va_list argp) {
  // Delegate formatting to string buffers. This makes the custom format handler
  // framework available when writing to any output stream, at least by default.
  // There's some extra overhead in doing it this way but if printing formatted
  // output ever becomes a bottleneck then probably there is a more serious
  // design flaw somewhere else that has led to heavy use of printf.
  string_buffer_t buf;
  if (!string_buffer_init(&buf))
    return 0;
  va_list next;
  va_copy(next, argp);
  if (!string_buffer_vprintf(&buf, fmt, next))
    return 0;
  va_end(next);
  utf8_t str = string_buffer_flush(&buf);
  size_t result = write_bytes(str.chars, str.size);
  string_buffer_dispose(&buf);
  return result;
}

bool StdioOpenFile::flush() {
  return fflush(file_) == 0;
}

bool out_stream_flush(out_stream_t *file) {
  return static_cast<OutStream*>(file)->flush();
}

size_t in_stream_read_bytes(in_stream_t *file, void *dest, size_t size) {
  return static_cast<InStream*>(file)->read_bytes(dest, size);
}

bool in_stream_at_eof(in_stream_t *file) {
  return static_cast<InStream*>(file)->at_eof();
}

size_t out_stream_vprintf(out_stream_t *file, const char *fmt, va_list argp) {
  va_list next;
  va_copy(next, argp);
  size_t result = static_cast<OutStream*>(file)->vprintf(fmt, next);
  va_end(next);
  return result;
}

size_t out_stream_printf(out_stream_t *file, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  size_t result = static_cast<OutStream*>(file)->vprintf(fmt, argp);
  va_end(argp);
  return result;
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

ByteInStream::ByteInStream(const void *data, size_t size)
  : data_(static_cast<const byte_t*>(data))
  , size_(size)
  , cursor_(0) { }

size_t ByteInStream::read_bytes(void *dest, size_t requested) {
  size_t remaining = size_ - cursor_;
  size_t block = (requested > remaining) ? remaining : requested;
  memcpy(dest, data_ + cursor_, block);
  cursor_ += block;
  return block;
}

bool ByteInStream::at_eof() {
  return cursor_ == size_;
}

in_stream_t *byte_in_stream_open(const void *data, size_t size) {
  return new ByteInStream(data, size);
}

void byte_in_stream_destroy(in_stream_t *stream) {
  delete static_cast<ByteInStream*>(stream);
}

ByteOutStream::ByteOutStream() { }

size_t ByteOutStream::write_bytes(const void *raw_src, size_t size) {
  const byte_t *src = static_cast<const byte_t*>(raw_src);
  for (size_t i = 0; i < size; i++)
    data_.push_back(src[i]);
  return size;
}

bool ByteOutStream::flush() {
  return true;
}
