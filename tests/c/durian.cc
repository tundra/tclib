//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// Durian is an executable that gets called to test the process abstraction. The
// name durian is arbitrary, I couldn't come up with a meaningful name that
// encapsulated what it did so, in the wave tradition, picked a random fruit.

#include "c/stdc.h"

class Durian {
public:
  static int main(int argc, char *argv[]);

  // Bridge to call the right isatty for the given platform.
  static bool is_a_tty(FILE *file);

  // Returns the environ array.
  static char **get_environment();

  // Are these two strings equal?
  static bool streq(const char *a, const char *b);
};

int Durian::main(int argc, char *argv[]) {
  int exit_code = 0;
  bool quiet = false;
  for (int i = 0; i < argc; ) {
    const char *next = argv[i++];
    if (streq(next, "--exit-code")) {
      exit_code = atoi(argv[i++]);
    } else if (streq(next, "--quiet")) {
      quiet = true;
    } else if (streq(next, "--getenv")) {
      const char *key = argv[i++];
      const char *value = getenv(key);
      printf("GETENV(%s): {%s}\n", key, value);
    } else if (streq(next, "--print-stderr")) {
      const char *value = argv[i++];
      fprintf(stderr, "%s", value);
    } else if (streq(next, "--echo-stdin")) {
      char buf[256];
      while (true) {
        size_t bytes_read = fread(buf, 1, 256, stdin);
        if (bytes_read > 0) {
          fwrite(buf, 1, bytes_read, stderr);
        } else if (feof(stdin)) {
          break;
        }
      }
    } else if ((strlen(next) >= 2) && (next[0] == '-') && (next[1] == '-')) {
      fprintf(stderr, "Unexpected option %s\n", next);
      fflush(stderr);
      exit(1);
    }
  }
  if (!quiet) {
    printf("ARGC: {%i}\n", argc);
    printf("ISATTY[in]: {%i}\n", is_a_tty(stdin));
    printf("ISATTY[out]: {%i}\n", is_a_tty(stdout));
    printf("ISATTY[err]: {%i}\n", is_a_tty(stderr));
    for (int i = 0; i < argc; i++)
      printf("ARGV[%i]: {%s}\n", i, argv[i]);
    for (char **ep = get_environment(); *ep; ep++)
      printf("ENV: {%s}\n", *ep);
  }
  return exit_code;
}

bool Durian::streq(const char *a, const char *b) {
  return strcmp(a, b) == 0;
}

int main(int argc, char *argv[]) {
  return Durian::main(argc, argv);
}

#ifdef IS_GCC
#  include <unistd.h>
#  define platform_isatty isatty
#else
#  include <io.h>
#  define platform_isatty _isatty
#endif

char **Durian::get_environment() {
  return environ;
}

bool Durian::is_a_tty(FILE *file) {
  return platform_isatty(fileno(file));
}
