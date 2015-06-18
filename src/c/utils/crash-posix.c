//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/stdc.h"
#include "io/file.h"
#include "utils/crash.h"
#include "utils/log.h"
#include "utils/string.h"

#define __USE_POSIX
#include <signal.h>

// --- S i g n a l   h a n d l i n g ---

// Print a stack trace if the platform supports it.
void print_stack_trace(out_stream_t *out, int signum);

// After handling the condition here, propagate it so that it doesn't get swallowed.
void propagate_condition(int signum);

// Optional platform-specific initialization code.
void initialize_crash_handler();

// Disable the signal handler such that nested signals go directly to the
// default handler rather than risk looping.
static void uninstall_signals() {
  signal(SIGSEGV, SIG_DFL);
  signal(SIGABRT, SIG_DFL);
}

// Processes crashes.
static void crash_handler(int signum) {
  uninstall_signals();
  file_system_t *fs = file_system_native();
  out_stream_t *out = file_system_stderr(fs);
  print_stack_trace(out, signum);
  out_stream_flush(out);
  propagate_condition(signum);
}

void install_crash_handler() {
  initialize_crash_handler();
  signal(SIGSEGV, crash_handler);
  signal(SIGABRT, crash_handler);
}

#ifdef IS_MSVC
#  include "crash-msvc.c"
#else
// For now always use execinfo. Later on some more logic can be built in to deal
// with the case where execinfo isn't available.
#  include "crash-execinfo.c"
#endif
