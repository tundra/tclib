//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Logging framework.


#ifndef _LOG
#define _LOG

#include "c/stdc.h"
#include "utils/ook.h"
#include "utils/string.h"
#include "fatbool.h"

#include <stdarg.h>

// Log statements below this level are stripped from the code.
#define LOG_LEVEL llInfo

// Which stream should the log entry be reported on?
typedef enum {
  lsStdout,
  lsStderr
} log_stream_t;

// What's the behavior after a message has been reported: keep going or abort?
typedef enum {
  lbAbort,
  lbContinue
} log_behavior_t;

// Call the given callback for each log level, log level character, log level
// value, and destination log stream. The more serious the error the higher the
// value will be.
//
//  Name     char value stream    action      fail exit code
#define ENUM_LOG_LEVELS(C)                                                     \
  C(Debug,   D,   0,    lsStdout, lbContinue, _)                               \
  C(Info,    I,   1,    lsStdout, lbContinue, _)                               \
  C(Warning, W,   2,    lsStderr, lbContinue, _)                               \
  C(Error,   E,   3,    lsStderr, lbContinue, X)                               \
  C(Fatal,   F,   4,    lsStderr, lbAbort,    X)                               \
  C(Hest,    H,   5,    lsStderr, lbContinue, _)

// Special log topics that can be turned on and off statically and dynamically.
// These are useful if you want to instrument particular areas of the code but
// have the logging off by default and be able to turn it on and off
// selectively. Since these are a debug aid they should always be returned to
// tlNever before submitting code.
#define LOG_TOPIC_Interpreter tlNever
#define LOG_TOPIC_Lookup      tlNever
#define LOG_TOPIC_Library     tlNever
#define LOG_TOPIC_Freeze      tlNever

// Topic log settings: always log, never log, dynamically toggle.
#define tlAlways  true
#define tlNever   false
#define tlDynamic dynamic_topic_logging_enabled

// Flag that controls whether topic logging is enabled in dynamic mode.
extern bool dynamic_topic_logging_enabled;

// Toggle whether topic logging is enabled for those topics that are set to
// tlDynamic. If you have multiple dynamic topics this doesn't allow you to
// control them separately but it's pretty easy to add if it becomes an issue.
// Returns the previous value in case you need to be able to restore the
// previous value.
bool set_topic_logging_enabled(bool value);

// Expands to a true value if the current log level is the specified value or
// less severe, implying that log messages to the given level should be
// reported.
#define LOG_LEVEL_AT_LEAST(llLevel) (LOG_LEVEL <= llLevel)

// Log levels, used to select which logging statements to emit.
typedef enum {
  __llFirst__ = -1
#define __DECLARE_LOG_LEVEL__(Name, C, V, S, B, E) , ll##Name = V
  ENUM_LOG_LEVELS(__DECLARE_LOG_LEVEL__)
#undef __DECLARE_LOG_LEVEL__
} log_level_t;

// Logs a message at the given log level.
void log_message(log_level_t level, const char *file, int line, const char *fmt,
    ...);

// Va_list version of log message.
fat_bool_t vlog_message(log_level_t level, const char *file, int line, const char *fmt,
    va_list argp);

// The data that makes up an entry in the log.
typedef struct {
  log_stream_t destination;
  log_behavior_t behavior;
  const char *file;
  int line;
  log_level_t level;
  utf8_t message;
  utf8_t timestamp;
} log_entry_t;

// Sets all the required fields in a log entry struct.
void log_entry_init(log_entry_t *entry, log_stream_t destination,
    log_behavior_t behavior, const char *file, int line, log_level_t level,
    utf8_t message, utf8_t timestamp);

// Initialize a log entry using defaults for the fields that can be derived from
// the given arguments, rather than specifying all the fields explicitly.
void log_entry_default_init(log_entry_t *entry, log_level_t level,
    const char *file, int line, utf8_t message, utf8_t timestamp);

// Logs a message that has already been processed into an entry.
fat_bool_t log_entry(log_entry_t *entry);

// Returns true if an entry has been logged whose log level requires the runtime
// to yield a non-successful error code.
bool has_logged_error_code_exit_entry();

INTERFACE(log_o);

// Type of log functions.
typedef fat_bool_t (*log_m)(log_o *self, log_entry_t *entry);

struct log_o_vtable_t {
  log_m log;
};

// A callback used to issue log messages.
struct log_o {
  INTERFACE_HEADER(log_o);
};

// Sets the log callback to use across this process. This should only be used
// for testing. Returns the previous value such that it can be restored if
// necessary.
log_o *set_global_log(log_o *log);

// Works the same as LOG_WARN but allows the file and line to be specified
// explicitly.
#define LOG_WARN_FILE_LINE(FILE, LINE, ...) do {                                           \
  if (LOG_LEVEL_AT_LEAST(llWarning))                                           \
    log_message(llWarning, FILE, LINE, __VA_ARGS__);                           \
} while (false)

// Emits a warning if the static log level is at least warning, otherwise does
// nothing (including doesn't evaluate arguments).
#define LOG_WARN(...) LOG_WARN_FILE_LINE(__FILE__, __LINE__, __VA_ARGS__)

#ifndef WARN
#  define WARN LOG_WARN
#endif

// Emits an error and aborts execution.
#define FATAL(...) do {                                                        \
  log_message(llFatal, __FILE__, __LINE__, __VA_ARGS__);                       \
} while (false)

// Emits an error if the static log level is at least error, otherwise does
// nothing (including doesn't evaluate arguments).
#define LOG_ERROR(...) do {                                                    \
  if (LOG_LEVEL_AT_LEAST(llError))                                             \
    log_message(llError, __FILE__, __LINE__, __VA_ARGS__);                     \
} while (false)

// Unless someone else has clobbered ERROR (*ahem-windows*) we define it.
#ifndef ERROR
#  define ERROR LOG_ERROR
#endif

// Emits an info if the static log level is at least info, otherwise does
// nothing (including doesn't evaluate arguments).
#define LOG_INFO(...) do {                                                     \
  if (LOG_LEVEL_AT_LEAST(llInfo))                                              \
    log_message(llInfo, __FILE__, __LINE__, __VA_ARGS__);                      \
} while (false)

#ifndef INFO
#  define INFO LOG_INFO
#endif

// Emits an debug if the static log level is at least debug (which it's not by
// default), otherwise does nothing (including doesn't evaluate arguments).
#define LOG_DEBUG(...) do {                                                    \
  if (LOG_LEVEL_AT_LEAST(llDebug))                                             \
    log_message(llDebug, __FILE__, __LINE__, __VA_ARGS__);                     \
} while (false)

#ifndef DEBUG
#  define DEBUG LOG_DEBUG
#endif

// Works the same way as INFO but doesn't print file:line such that the output
// is the same regardless of how the binary has been compiled. Useful for tests.
#define INFO_DETERMINISTIC(...) do {                                           \
  if (LOG_LEVEL_AT_LEAST(llInfo))                                              \
    log_message(llInfo, NULL, __LINE__, __VA_ARGS__);                          \
} while (false)

// Log an event relevant to the given topic. If logging is disabled for the
// given topic nothing happens.
#define TOPIC_INFO(TOPIC, ...) do {                                            \
  if (LOG_TOPIC_##TOPIC)                                                       \
    INFO(__VA_ARGS__);                                                         \
} while (false)

// Works the same as INFO but any occurrences of this macro will cause
// compilation to fail if devutils are disallowed. If you use this when
// debugging/tracing by hest this will help you remember to get rid of all the
// debug print statements before submitting.
#ifdef ALLOW_DEVUTILS
#  define HEST(...) log_message(llHest, __FILE__, __LINE__, __VA_ARGS__)
#else
#  define HEST(...) devutils_not_allowed
#endif

#endif // _LOG
