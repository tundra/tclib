//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "io/stream.hh"
#include "io/file.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
#include "utils/strbuf.h"
END_C_INCLUDES

using namespace tclib;

bool AbstractStream::close() {
  return true;
}

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
  WriteIop iop(this, str.chars, str.size);
  iop.execute();
  string_buffer_dispose(&buf);
  return iop.bytes_written();
}

bool out_stream_flush(out_stream_t *stream) {
  return static_cast<OutStream*>(stream)->flush();
}

bool out_stream_close(out_stream_t *stream) {
  return static_cast<OutStream*>(stream)->close();
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

ByteInStream::ByteInStream(const void *data, size_t size)
  : data_(static_cast<const byte_t*>(data))
  , size_(size)
  , cursor_(0) { }

bool ByteInStream::read_sync(read_iop_state_t *iop) {
  size_t remaining = size_ - cursor_;
  size_t block = (iop->dest_size_ > remaining) ? remaining : iop->dest_size_;
  memcpy(iop->dest_, data_ + cursor_, block);
  cursor_ += block;
  read_iop_state_deliver(iop, block, cursor_ == size_);
  return true;
}

in_stream_t *byte_in_stream_open(const void *data, size_t size) {
  return new ByteInStream(data, size);
}

void byte_in_stream_destroy(in_stream_t *stream) {
  delete static_cast<ByteInStream*>(stream);
}

ByteOutStream::ByteOutStream() { }

void ByteOutStream::putchar(byte_t c) {
  data_.push_back(c);
}

bool ByteOutStream::write_sync(write_iop_state_t *op) {
  const byte_t *src = static_cast<const byte_t*>(op->src);
  size_t size = op->src_size;
  for (size_t i = 0; i < size; i++)
    putchar(src[i]);
  op->bytes_written = size;
  return true;
}

bool ByteOutStream::flush() {
  return true;
}

#ifdef IS_GCC
#  include "stream-posix.cc"
#else
#  include "stream-msvc.cc"
#endif
