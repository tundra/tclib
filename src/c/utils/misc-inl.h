//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _UTILS_MISC_H
#define _UTILS_MISC_H

#include "c/stdc.h"

always_inline bool is_power_of_2(uint64_t value) {
  return (value == 1ULL) || !(value & (value - 1));
}

#endif // _UTILS_MISC_H
