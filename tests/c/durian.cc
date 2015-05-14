//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Durian is an executable that gets called to test the process abstraction. The
// name durian is arbitrary, I couldn't come up with a meaningful name that
// encapsulated what it did so, in the wave tradition, picked a random fruit.

#include "c/stdc.h"

typedef struct {
  int exit_code;
} durian_options_t;

// Parse command-line options.
static durian_options_t parse_options(int argc, char *argv[]) {
  durian_options_t options;
  options.exit_code = 0;
  for (int i = 0; i < argc; ) {
    const char *next = argv[i++];
    if (strcmp(next, "--exit-code") == 0)
      options.exit_code = atoi(argv[i++]);
  }
  return options;
}

// By convention this is available even on windows. See man environ.
extern char **environ;

int main(int argc, char *argv[]) {
  durian_options_t options = parse_options(argc, argv);
  printf("ARGC: {%i}\n", argc);
  for (int i = 0; i < argc; i++)
    printf("ARGV[%i]: {%s}\n", i, argv[i]);
  for (char **ep = environ; *ep; ep++)
    printf("ENV: {%s}\n", *ep);
  return options.exit_code;
}
