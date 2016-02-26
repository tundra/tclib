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

// Expands to an expression that yields the given size rounded up to the nearest
// multiple of the given alignment. This evaluates the alignment twice and may
// mess with the types involved (64/32 bits) so if at all possible use
// align_size instead.
#define STATIC_ALIGN_SIZE(ALGN, SIZE) (((SIZE) + ((ALGN) - 1)) & (~((ALGN) - 1)))

// Returns a pointer greater than or equal to the given pointer which is
// aligned to an 'alignment' boundary.
static size_t align_size(size_t alignment, size_t size) {
  return STATIC_ALIGN_SIZE(alignment, size);
}

// Returns the greatest of a and b.
static always_inline size_t max_size(size_t a, size_t b) {
  return (a < b) ? b : a;
}

// Returns the smallest of a and b.
static always_inline size_t min_size(size_t a, size_t b) {
  return (a < b) ? a : b;
}

// Returns the greatest of a and b.
static always_inline int64_t max_64(int64_t a, int64_t b) {
  return (a < b) ? b : a;
}

// Returns the smallest of a and b.
static always_inline int64_t min_64(int64_t a, int64_t b) {
  return (a < b) ? a : b;
}

#endif // _UTILS_MISC_H
