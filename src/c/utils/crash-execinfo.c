//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Implementation of crash dumps that uses execinfo.

#include <execinfo.h>
#include <unistd.h>

static const size_t kMaxStackSize = 128;

void initialize_crash_handler() {
  // nothing to initialize
}

void print_stack_trace(open_file_t *out, int signum) {
  open_file_printf(out, "# Received condition %i\n", signum);
  void *raw_frames[kMaxStackSize];
  size_t size = backtrace(raw_frames, kMaxStackSize);
  char **frames = backtrace_symbols(raw_frames, size);
  for (size_t i = 0; i < size; i++) {
    open_file_printf(out, "# - %s\n", frames[i]);
  }
  free(frames);
  open_file_flush(out);
}

void propagate_condition(int signum) {
  kill(getpid(), signum);
}
