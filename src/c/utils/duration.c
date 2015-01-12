//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "duration.h"

void duration_add_to_timespec(duration_t duration, uint64_t *sec, uint64_t *nsec) {
  static const uint64_t kNsPerS = 1000000000;
  uint64_t millis = duration.millis;
  *sec += (millis / 1000);
  *nsec += (millis % 1000) * 1000000;
  if (*nsec >= kNsPerS) {
    (*sec)++;
    (*nsec) -= kNsPerS;
  }
}

