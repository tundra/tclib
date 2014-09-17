//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "check.h"
#include "strbuf.h"

void string_buffer_init(string_buffer_t *buf) {
  buf->length = 0;
  buf->memory = allocator_default_malloc(128);
}

void string_buffer_dispose(string_buffer_t *buf) {
  allocator_default_free(buf->memory);
}

// Expands the buffer to make room for 'length' characters if necessary.
static void string_buffer_ensure_capacity(string_buffer_t *buf,
    size_t length) {
  if (length < buf->memory.size)
    return;
  size_t new_capacity = (length * 2);
  memory_block_t new_memory = allocator_default_malloc(new_capacity);
  memcpy(new_memory.memory, buf->memory.memory, buf->length);
  allocator_default_free(buf->memory);
  buf->memory = new_memory;
}

void string_buffer_append(string_buffer_t *buf, string_t *str) {
  string_buffer_ensure_capacity(buf, buf->length + string_length(str));
  char *chars = (char*) buf->memory.memory;
  string_copy_to(str, chars + buf->length, buf->memory.size - buf->length);
  buf->length += string_length(str);
}

void string_buffer_putc(string_buffer_t *buf, char c) {
  string_buffer_ensure_capacity(buf, buf->length + 1);
  char *chars = (char*) buf->memory.memory;
  chars[buf->length] = c;
  buf->length++;
}

void string_buffer_printf(string_buffer_t *buf, const char *fmt, ...) {
  va_list argp;
  va_start(argp, fmt);
  string_buffer_vprintf(buf, fmt, argp);
  va_end(argp);
}

void string_buffer_native_printf(string_buffer_t *buf, const char *fmt, ...) {
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
  string_t data = {written, buffer};
  string_buffer_append(buf, &data);
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

void string_buffer_vprintf(string_buffer_t *buf, const char *fmt, va_list argp) {
  // This is incredibly tedious code but in the absence of a reliable way to
  // introduce new format types this seems like the best way to allow custom
  // format types in a way that localizes the complexity here rather than
  // spreading it everywhere this is used.
  for (const char *p = fmt; *p != '\0'; p++) {
    if (*p == '%') {
      p++;
      char c = *p;
      // Read any leading integer parameters.
      int32_t int_param = -1;
      if ('0' <= c && c <= '9') {
        int_param = 0;
        while ('0' <= c && c <= '9') {
          int_param = (10 * int_param) + (c - '0');
          p++;
          c = *p;
        }
      }
      // Count leading 'l's.
      size_t l_count = 0;
      while (c == 'l') {
        l_count++;
        p++;
        c = *p;
      }
      switch (c) {
        case 's': {
          // Ideally the string's length would be given somehow but alas that's
          // not really possibly through var args.
          const char *c_str = va_arg(argp, const char *);
          string_t str;
          string_init(&str, c_str);
          string_buffer_append(buf, &str);
          break;
        }
        case 'i':
          switch (l_count) {
            case 0: {
              int value = va_arg(argp, int);
              string_buffer_native_printf(buf, "%i", value);
              break;
            }
            case 1: {
              long value = va_arg(argp, long);
              string_buffer_native_printf(buf, "%li", value);
              break;
            }
            case 2: {
              long long value = va_arg(argp, long long);
              string_buffer_native_printf(buf, "%lli", value);
              break;
            }
            default:
              // Emit what we just read since we couldn't make sense of it.
              string_buffer_putc(buf, '%');
              for (size_t i = 0; i < l_count; i++)
                string_buffer_putc(buf, 'l');
              string_buffer_putc(buf, 'i');
              break;
          }
          break;
        case 'f': {
          double value = (double) va_arg(argp, double);
          string_buffer_native_printf(buf, "%f", value);
          break;
        }
        case 'p': {
          void *ptr = (void*) va_arg(argp, void*);
          string_buffer_native_printf(buf, "%p", ptr);
          break;
        }
        case 'c': {
          // Assume the char has been fully promoted to an int.
          char value = (char) va_arg(argp, int);
          string_buffer_native_printf(buf, "%c", value);
          break;
        }
        case '%':
          string_buffer_putc(buf, '%');
          break;
        default: {
          format_handler_o **handler = get_format_handler_ref(c);
          if (handler == NULL || (*handler) == NULL) {
            string_buffer_native_printf(buf, "%%%c", c);
          } else {
            format_request_t request = {buf, int_param, c};
            METHOD(*handler, write_format_value)(*handler, &request, VA_LIST_REF(argp));
          }
          break;
        }
      }
    } else {
      string_buffer_putc(buf, *p);
    }
  }
}

void string_buffer_flush(string_buffer_t *buf, string_t *str_out) {
  CHECK_REL("no room for null terminator", buf->length, <, buf->memory.size);
  char *chars = (char*) buf->memory.memory;
  chars[buf->length] = '\0';
  str_out->length = buf->length;
  str_out->chars = chars;
}

void register_format_handler(char c, format_handler_o *handler) {
  format_handler_o **ref = get_format_handler_ref(c);
  if (ref != NULL && *ref == NULL)
    *ref = handler;
}
