//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_STRBUF_H
#define _TCLIB_STRBUF_H

#include "c/stdc-inl.h"
#include "utils/alloc.h"
#include "utils/fatbool.h"
#include "utils/ook.h"
#include "utils/string.h"


// Buffer for building a string incrementally.
typedef struct {
  // Size of string currently in the buffer.
  size_t length;
  // The data buffer.
  blob_t memory;
} string_buffer_t;

// Initialize a string buffer.
fat_bool_t string_buffer_init(string_buffer_t *buf);

// Disposes the given string buffer.
void string_buffer_dispose(string_buffer_t *buf);

// Add a single character to this buffer.
fat_bool_t string_buffer_putc(string_buffer_t *buf, char c);

// Append the given text to the given buffer.
fat_bool_t string_buffer_printf(string_buffer_t *buf, const char *format, ...);

// Format the given input onto the given buffer using native sprintf.
fat_bool_t string_buffer_native_printf(string_buffer_t *buf, const char *fmt, ...);

// Append the given text to the given buffer.
fat_bool_t string_buffer_vprintf(string_buffer_t *buf, const char *format, va_list argp);

// Append the contents of the string to this buffer.
fat_bool_t string_buffer_append(string_buffer_t *buf, utf8_t str);

// Null-terminates the buffer and stores the result in the given out parameter.
// The string is still backed by the buffer and so becomes invalid when the
// buffer is disposed.
utf8_t string_buffer_flush(string_buffer_t *buf);

INTERFACE(format_handler_o);

// Flags that can be set in a format request to control how values are printed.
typedef enum {
  ffDash = (1 << 0),
  ffPlus = (1 << 1),
  ffSpace = (1 << 2),
  ffZero = (1 << 3),
  ffHash = (1 << 4)
} format_flag_t;

// Format flag characters. The order must match the enum values, so the enum
// value corresponding to the i'th format char must have value (1 << i).
#define kPrintfFormatFlags "-+ 0#"

typedef struct {
  // The buffer to write the output on.
  string_buffer_t *buf;
  // Optional width parameter given to the format character. If no width was
  // specified this will be -1.
  int32_t width;
  // Any format flags in the input, or'ed together.
  int32_t flags;
  // The format character itself.
  char format;
} format_request_t;

// Type of format-write functions.
typedef fat_bool_t (*write_format_value_m)(format_handler_o *self,
    format_request_t *request, va_list_ref_t argp);

struct format_handler_o_vtable_t {
  write_format_value_m write_format_value;
};

// A callback used to abort execution.
struct format_handler_o {
  INTERFACE_HEADER(format_handler_o);
};

// Registers a string format handler under the given character. The character
// must be unbound.
void register_format_handler(char c, format_handler_o *handler);

// Clears the handler registered under the given character.
void unregister_format_handler(char c);

#endif // _TCLIB_STRBUF_H
