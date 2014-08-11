//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "unittest.hh"

#include <stdarg.h>

// Default implementation of failure that prints to stderr and then aborts.
void fail(const char *file, int line, const char *fmt, ...) {
  fprintf(stderr, "%s:%i: ", file, line);
  va_list argp;
  va_start(argp, fmt);
  vfprintf(stderr, fmt, argp);
  va_end(argp);
  abort();
}
