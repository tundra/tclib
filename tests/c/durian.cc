//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Durian is an executable that gets called to test the process abstraction. The
// name durian is arbitrary, I couldn't come up with a meaningful name that
// encapsulated what it did so, in the wave tradition, picked a random fruit.

#include "c/stdc.h"

// By convention this is available even on windows. See man environ.
extern char **environ;

int main(int argc, char *argv[]) {
  printf("ARGC: {%i}\n", argc);
  for (int i = 0; i < argc; i++)
    printf("ARGV[%i]: {%s}\n", i, argv[i]);
  for (char **ep = environ; *ep; ep++)
    printf("ENV: {%s}\n", *ep);
  int exit_code = 0;
  for (int i = 0; i < argc; ) {
    const char *next = argv[i++];
    if (strcmp(next, "--exit-code") == 0) {
      exit_code = atoi(argv[i++]);
    } else if (strcmp(next, "--getenv") == 0) {
      const char *key = argv[i++];
      const char *value = getenv(key);
      printf("GETENV(%s): {%s}\n", key, value);
    } else if (strcmp(next, "--print-stderr") == 0) {
      const char *value = argv[i++];
      fprintf(stderr, "%s\n", value);
    }
  }
  return exit_code;
}
