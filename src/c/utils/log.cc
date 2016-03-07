//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/stdc.h"

#include "utils/log.hh"
#include "utils/fatbool.hh"

BEGIN_C_INCLUDES
#include "io/file.h"
#include "utils/ook.h"
#include "utils/strbuf.h"
#include "utils/string-inl.h"
#include "utils/trybool.h"
END_C_INCLUDES

using namespace tclib;

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
#define __LEVEL_CASE__(Name, C, V, S, B, E) case ll##Name: return #C;
    ENUM_LOG_LEVELS(__LEVEL_CASE__)
#undef __LEVEL_CASE__
    default:
      return "?";
  }
}

// Returns the full name of the given log level.
static const char *get_log_level_name(log_level_t level) {
  switch (level) {
#define __LEVEL_CASE__(Name, C, V, S, B, E) case ll##Name: return #Name;
    ENUM_LOG_LEVELS(__LEVEL_CASE__)
#undef __LEVEL_CASE__
    default:
      return "?";
  }
}

// Returns the destination stream of the given log level.
static log_stream_t get_log_level_destination(log_level_t level) {
  switch (level) {
#define __LEVEL_CASE__(Name, C, V, S, B, E) case ll##Name: return S;
    ENUM_LOG_LEVELS(__LEVEL_CASE__)
#undef __LEVEL_CASE__
    default:
      return lsStderr;
  }
}

// Returns the destination stream of the given log level.
static log_behavior_t get_log_level_behavior(log_level_t level) {
  switch (level) {
#define __LEVEL_CASE__(Name, C, V, S, B, E) case ll##Name: return B;
    ENUM_LOG_LEVELS(__LEVEL_CASE__)
#undef __LEVEL_CASE__
    default:
      return lbContinue;
  }
}

// When we log at the given log level, should that automatically mean that the
// return code from the runtime indicates error?
static bool get_log_level_fail_exit_code(log_level_t level) {
  switch (level) {
#define __LEVEL_CASE__(Name, C, V, S, B, E) E(case ll##Name:,)
    ENUM_LOG_LEVELS(__LEVEL_CASE__)
#undef __LEVEL_CASE__
      return true;
    default:
      return false;
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

static bool has_logged_error_code_exit_entry_ = false;

// The default abort handler which prints the message to stdout/err and aborts
// execution.
static bool default_log(log_o *log, log_entry_t *entry) {
  out_stream_t *dest = (entry->destination == lsStderr)
      ? file_system_stderr(file_system_native())
      : file_system_stdout(file_system_native());
  if (entry->behavior == lbAbort) {
    // If we're aborting then abort takes care of printing the message -- we'll
    // actually get back here in a few calls but the behavior will have been
    // changed to continue and then we'll fall through to the other cases.
    abort_message_t message;
    abort_message_init(&message, entry->destination, entry->file, entry->line,
        0, entry->message.chars);
    abort_call(get_global_abort(), &message);
  } else if (entry->file == NULL) {
    // This is typically used for testing where including the filename and line
    // makes the output unpredictable.
    out_stream_printf(dest, "%s: %s\n",
        get_log_level_name(entry->level), entry->message.chars);
  } else {
    out_stream_printf(dest, "%s:%i: %s: %s [%s%s]\n", entry->file, entry->line,
        get_log_level_name(entry->level), entry->message.chars,
        get_log_level_char(entry->level), entry->timestamp.chars);
  }
  if (get_log_level_fail_exit_code(entry->level))
    has_logged_error_code_exit_entry_ = true;
  out_stream_flush(dest);
  return true;
}

VTABLE(default_log_o, log_o) { default_log };

bool has_logged_error_code_exit_entry() {
  return has_logged_error_code_exit_entry_;
}

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
    log_behavior_t behavior, const char *file, int line, log_level_t level,
    utf8_t message, utf8_t timestamp) {
  entry->destination = destination;
  entry->behavior = behavior;
  entry->file = file;
  entry->line = line;
  entry->level = level;
  entry->message = message;
  entry->timestamp = timestamp;
}

void log_entry_default_init(log_entry_t *entry, log_level_t level,
    const char *file, int line, utf8_t message, utf8_t timestamp) {
  log_entry_init(entry, get_log_level_destination(level),
      get_log_level_behavior(level), file, line, level, message, timestamp);
}

bool vlog_message(log_level_t level, const char *file, int line, const char *fmt,
    va_list argp) {
  // Write the error message into a string buffer.
  string_buffer_t buf;
  B_TRY(string_buffer_init(&buf));
  B_TRY(string_buffer_vprintf(&buf, fmt, argp));
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
  // Print the result.
  log_entry_t entry;
  log_entry_default_init(&entry, level, file, line, message_str, timestamp_str);
  B_TRY(log_entry(&entry));
  string_buffer_dispose(&buf);
  return true;
}

bool log_entry(log_entry_t *entry) {
  log_o *log = get_global_log();
  return METHOD(log, log)(log, entry);
}

log_o_vtable_t Log::kVTable = {log_trampoline};

bool Log::log_trampoline(log_o *self, log_entry_t *entry) {
  return static_cast<Log*>(self)->record(entry);
}

Log::Log()
  : outer_(NULL) {
  header.vtable = &kVTable;
}

void Log::ensure_installed() {
  if (is_installed())
    return;
  outer_ = set_global_log(this);
}

void Log::ensure_uninstalled() {
  if (!is_installed())
    return;
  set_global_log(outer_);
  outer_ = NULL;
}

bool Log::propagate(log_entry_t *entry) {
  return METHOD(outer_, log)(outer_, entry);
}

void fat_bool_log_failure(const char *file, int line, fat_bool_t error) {
  LOG_WARN_FILE_LINE(file, line, "Propagating fatbool " kFatBoolFileLine,
      fat_bool_file(error), fat_bool_line(error));
}
