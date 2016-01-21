//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _UTILS_MISC_H
#define _UTILS_MISC_H

#include "c/stdc.h"

// Returns true iff the given value is a power of 2. Arbitrarily returns true on
// 0.
static always_inline bool is_power_of_2(uint64_t value) {
  return (value == 1ULL) || !(value & (value - 1));
}

// Returns the given value rounded up to the nearest multiple of the given
// alignment which must be a power of 2.
static always_inline uint64_t align_uint64(uint64_t alignment, uint64_t value) {
  return (value + (alignment - 1ULL)) & (~(alignment - 1ULL));
}

#endif // _UTILS_MISC_H
