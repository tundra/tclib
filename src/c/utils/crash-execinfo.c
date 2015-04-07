//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Implementation of crash dumps that uses execinfo.

#include <execinfo.h>
#include <unistd.h>

static const int kMaxStackSize = 128;

void initialize_crash_handler() {
  // nothing to initialize
}

void print_stack_trace(out_stream_t *out, int signum) {
  out_stream_printf(out, "# Received condition %i\n", signum);
  void *raw_frames[kMaxStackSize];
  int size = backtrace(raw_frames, kMaxStackSize);
  char **frames = backtrace_symbols(raw_frames, size);
  for (int i = 0; i < size; i++) {
    out_stream_printf(out, "# - %s\n", frames[i]);
  }
  free(frames);
  out_stream_flush(out);
}

void propagate_condition(int signum) {
  kill(getpid(), signum);
}
