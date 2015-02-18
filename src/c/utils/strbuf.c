//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "check.h"
#include "strbuf.h"
#include "string-inl.h"
#include "utils/trybool.h"

bool string_buffer_init(string_buffer_t *buf) {
  buf->length = 0;
  buf->memory = allocator_default_malloc(128);
  return !memory_block_is_empty(buf->memory);
}

void string_buffer_dispose(string_buffer_t *buf) {
  allocator_default_free(buf->memory);
}

// Expands the buffer to make room for 'length' characters if necessary.
static bool string_buffer_ensure_capacity(string_buffer_t *buf,
    size_t length) {
  if (length < buf->memory.size)
    return true;
  size_t new_capacity = (length * 2);
  memory_block_t new_memory = allocator_default_malloc(new_capacity);
  if (memory_block_is_empty(new_memory))
    return false;
  memcpy(new_memory.memory, buf->memory.memory, buf->length);
  allocator_default_free(buf->memory);
  buf->memory = new_memory;
  return true;
}

bool string_buffer_append(string_buffer_t *buf, utf8_t str) {
  B_TRY(string_buffer_ensure_capacity(buf, buf->length + string_size(str)));
  char *chars = (char*) buf->memory.memory;
  string_copy_to(str, chars + buf->length, buf->memory.size - buf->length);
  buf->length += string_size(str);
  return true;
}

bool string_buffer_putc(string_buffer_t *buf, char c) {
  B_TRY(string_buffer_ensure_capacity(buf, buf->length + 1));
  char *chars = (char*) buf->memory.memory;
  chars[buf->length] = c;
  buf->length++;
  return true;
}

bool string_buffer_printf(string_buffer_t *buf, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  bool result = string_buffer_vprintf(buf, fmt, argp);
  va_end(argp);
  return result;
}

bool string_buffer_native_printf(string_buffer_t *buf, const char *fmt, ...) {
  // Write the formatted string into a temporary buffer.
  static const size_t kMaxSize = 1024;
  char buffer[kMaxSize + 1];
  // Null terminate explicitly just to be on the safe side.
  buffer[kMaxSize] = '\0';
  va_list argp;
  va_start(argp, fmt);
  size_t written = vsnprintf(buffer, kMaxSize, fmt, argp);
  va_end(argp);
  // TODO: fix this if we ever hit it.
  CHECK_REL("temp buffer too small", written, <, kMaxSize);
  // Then write the temp string into the string buffer.
  utf8_t data = {written, buffer};
  B_TRY(string_buffer_append(buf, data));
  return true;
}

static format_handler_o **get_format_handler_ref(int c) {
  static format_handler_o *format_handler_memory[256];
  static format_handler_o **format_handlers = NULL;
  if (!(0 <= c && c < 256))
    return NULL;
  if (format_handlers == NULL) {
    for (size_t i = 0; i < 256; i++)
      format_handler_memory[i] = NULL;
    format_handlers = format_handler_memory;
  }
  return &format_handlers[c];
}

static const char *kPrintfFlagChars = "-+ 0#";
static const char *kPrintfWidthChars = "0123456789";
static const char *kPrintfPrecisionChars = "0123456789";
static const char *kPrintfLengthModifierChars = "lLh";

// Does the given string contain the given character?
static bool string_contains(const char *str, char c) {
  return strchr(str, c) != NULL;
}

bool string_buffer_vprintf(string_buffer_t *buf, const char *fmt, va_list argp) {
  // This is incredibly tedious code but in the absence of a reliable way to
  // introduce new format types this seems like the best way to allow custom
  // format types in a way that localizes the complexity here rather than
  // spreading it everywhere this is used.
  //
  // Based on K&R B1.2: Formatted Output.
  for (const char *p = fmt; *p != '\0'; p++) {
    if (*p == '%') {
      const char *start = p;
      p++;
      int width = -1;
      size_t ls = 0, Ls = 0;
      // Skip across the flags; they don't really matter here.
      while (*p && string_contains(kPrintfFlagChars, *p))
        p++;
      // Skip across the width, recording it in the width variable so it can be
      // passed to the custom formatters.
      while (*p && string_contains(kPrintfWidthChars, *p)) {
        if (width == -1)
          // Only initialize to 0 if there is an actual width.
          width = 0;
        width = (10 * width) + (*p - '0');
        p++;
      }
      if (*p == '.') {
        // Skip the precision, again we don't really care here.
        p++;
        while (*p && string_contains(kPrintfPrecisionChars, *p))
          p++;
      }
      // Skip the length modifiers and keep track of the one that influence the
      // width of the next argument.
      while (*p && string_contains(kPrintfLengthModifierChars, *p)) {
        switch (*p) {
          case 'l': ls++; break;
          case 'L': Ls++; break;
        }
        p++;
      }
      char c = *p;
      char fmt_buf[256];
      size_t fmt_size = p - start + 1;
      if (fmt_size > 255)
        return false;
      memcpy(fmt_buf, start, fmt_size);
      fmt_buf[fmt_size] = '\0';
      switch (c) {
        case 'd': case 'i': case 'o': case 'x': case 'X': case 'u': case 'c':
          if (ls == 0) {
            B_TRY(string_buffer_native_printf(buf, fmt_buf, va_arg(argp, int)));
          } else if (ls == 1) {
            B_TRY(string_buffer_native_printf(buf, fmt_buf, va_arg(argp, long)));
          } else {
            B_TRY(string_buffer_native_printf(buf, fmt_buf, va_arg(argp, long long)));
          }
          break;
        case 's': {
          B_TRY(string_buffer_native_printf(buf, fmt_buf, va_arg(argp, char*)));
          break;
        }
        case 'f': case 'e': case 'E': case 'g': case 'G': {
          if (Ls == 0) {
            B_TRY(string_buffer_native_printf(buf, fmt_buf, va_arg(argp, double)));
          } else {
            B_TRY(string_buffer_native_printf(buf, fmt_buf, va_arg(argp, long double)));
          }
          break;
        }
        case 'p': {
          B_TRY(string_buffer_native_printf(buf, fmt_buf, va_arg(argp, void*)));
          break;
        }
        case 'n': {
          B_TRY(string_buffer_native_printf(buf, fmt_buf, va_arg(argp, int*)));
          break;
        }
        case '%':
          B_TRY(string_buffer_putc(buf, '%'));
          break;
        default: {
          format_handler_o **handler = get_format_handler_ref(c);
          if (handler == NULL || (*handler) == NULL) {
            B_TRY(string_buffer_native_printf(buf, "%%%c", c));
          } else {
            format_request_t request = {buf, width, c};
            METHOD(*handler, write_format_value)(*handler, &request, VA_LIST_REF(argp));
          }
          break;
        }
      }
    } else {
      B_TRY(string_buffer_putc(buf, *p));
    }
  }
  return true;
}

utf8_t string_buffer_flush(string_buffer_t *buf) {
  CHECK_REL("no room for null terminator", buf->length, <, buf->memory.size);
  char *chars = (char*) buf->memory.memory;
  chars[buf->length] = '\0';
  return new_string(chars, buf->length);
}

void register_format_handler(char c, format_handler_o *handler) {
  format_handler_o **ref = get_format_handler_ref(c);
  if (ref != NULL && *ref == NULL)
    *ref = handler;
}

void unregister_format_handler(char c) {
  format_handler_o **ref = get_format_handler_ref(c);
  if (ref != NULL)
    *ref = NULL;
}
