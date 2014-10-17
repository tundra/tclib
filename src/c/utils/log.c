//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "io/file.h"
#include "log.h"
#include "ook.h"
#include "strbuf.h"
#include "string-inl.h"

#include <time.h>

bool dynamic_topic_logging_enabled = false;

bool set_topic_logging_enabled(bool value) {
  bool prev_value = dynamic_topic_logging_enabled;
  dynamic_topic_logging_enabled = value;
  return prev_value;
}

// Returns the initial for the given log level.
static const char *get_log_level_char(log_level_t level) {
  switch (level) {
#define __LEVEL_CASE__(Name, C, V, S) case ll##Name: return #C;
    ENUM_LOG_LEVELS(__LEVEL_CASE__)
#undef __LEVEL_CASE__
    default:
      return "?";
  }
}

// Returns the full name of the given log level.
static const char *get_log_level_name(log_level_t level) {
  switch (level) {
#define __LEVEL_CASE__(Name, C, V, S) case ll##Name: return #Name;
    ENUM_LOG_LEVELS(__LEVEL_CASE__)
#undef __LEVEL_CASE__
    default:
      return "?";
  }
}

// Returns the destination stream of the given log level.
static log_stream_t get_log_level_destination(log_level_t level) {
  switch (level) {
#define __LEVEL_CASE__(Name, C, V, S) case ll##Name: return S;
    ENUM_LOG_LEVELS(__LEVEL_CASE__)
#undef __LEVEL_CASE__
    default:
      return lsStderr;
  }
}

void log_message(log_level_t level, const char *file, int line, const char *fmt,
    ...) {
  va_list argp;
  va_start(argp, fmt);
  vlog_message(level, file, line, fmt, argp);
  va_end(argp);
}

IMPLEMENTATION(default_log_o, log_o);

static log_o kDefaultLog;
static log_o *global_log = NULL;

// The default abort handler which prints the message to stderr and aborts
// execution.
static void default_log(log_o *log, log_entry_t *entry) {
  open_file_t *dest = file_system_stderr(file_system_native());
  if (entry->file == NULL) {
    // This is typically used for testing where including the filename and line
    // makes the output unpredictable.
    open_file_printf(dest, "%s: %s\n",
        get_log_level_name(entry->level), entry->message.chars);
  } else {
    open_file_printf(dest, "%s:%i: %s: %s [%s%s]\n", entry->file, entry->line,
        get_log_level_name(entry->level), entry->message.chars,
        get_log_level_char(entry->level), entry->timestamp.chars);
  }
  open_file_flush(dest);
}

VTABLE(default_log_o, log_o) { default_log };

// Returns the current global abort callback.
static log_o *get_global_log() {
  if (global_log == NULL) {
    VTABLE_INIT(default_log_o, &kDefaultLog);
    global_log = &kDefaultLog;
  }
  return global_log;
}

log_o *set_global_log(log_o *value) {
  log_o *result = get_global_log();
  global_log = value;
  return result;
}

void log_entry_init(log_entry_t *entry, log_stream_t destination,
    const char *file, int line, log_level_t level, utf8_t message,
    utf8_t timestamp) {
  entry->destination = destination;
  entry->file = file;
  entry->line = line;
  entry->level = level;
  entry->message = message;
  entry->timestamp = timestamp;
}


void vlog_message(log_level_t level, const char *file, int line, const char *fmt,
    va_list argp) {
  // Write the error message into a string buffer.
  string_buffer_t buf;
  string_buffer_init(&buf);
  string_buffer_vprintf(&buf, fmt, argp);
  va_end(argp);
  // Flush the string buffer.
  utf8_t message_str = string_buffer_flush(&buf);
  // Format the timestamp.
  time_t current_time;
  time(&current_time);
  struct tm local_time = *localtime(&current_time);
  char timestamp[128];
  size_t timestamp_chars = strftime(timestamp, 128, "%d%m%H%M%S", &local_time);
  utf8_t timestamp_str = new_string(timestamp, timestamp_chars);
  log_stream_t destination = get_log_level_destination(level);
  // Print the result.
  log_entry_t entry;
  log_entry_init(&entry, destination, file, line, level, message_str,
      timestamp_str);
  log_o *log = get_global_log();
  METHOD(log, log)(log, &entry);
  string_buffer_dispose(&buf);
}
