//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _OPAQUE_H
#define _OPAQUE_H

#include "c/stdc.h"

/// Generic opaque datatype, because void* isn't always wide enough.
///
/// An opaque is 64 bits wide so it can hold a pointer, a 64-bit int, or any
/// other 64-bit wide type. Really we could just use a naked uint64_t but using
/// an opaque explicitly avoids implicit conversions and makes it clearer what
/// the intention is.
///
/// Using an opaque is one-way: you can convert any of the supported types to an
/// opaque and then get the same value back, but you can't convert an opaque_t
/// to all the supported types and expect to be able to get the same opaque
/// back since some of those types are too narrow to hold a full opaque. You can
/// encode an opaque as a uint64 though and convert it back again withou loss.
///
/// Very unusually the methods that operate on opaques have really short names
/// to make them less cumbersome, which they otherwise very easily become.
typedef union {
  uint64_t as_uint64;
  void *as_voidp;
} opaque_t;

// Wrap a pointer in an opaque.
static always_inline opaque_t p2o(void *value) {
  opaque_t result;
  result.as_uint64 = 0;
  result.as_voidp = value;
  return result;
}

// Extract a pointer from an opaque. Note that converting the pointer back into
// an opaque is *not* guaranteed to produce the same opaque.
static always_inline void *o2p(opaque_t opaque) {
  return opaque.as_voidp;
}

// Wrap an uint64 in an opaque.
static always_inline opaque_t u2o(uint64_t value) {
  opaque_t result;
  result.as_uint64 = value;
  return result;
}

// Extract an uint64 from an opaque. Converting the result back into an opaque
// using u2o is guaranteed to produce the same opaque, even if it was originally
// not created from an uint64.
static always_inline uint64_t o2u(opaque_t opaque) {
  return opaque.as_uint64;
}

// Returns a null opaque value, that is, a value whose pointer value is NULL and
// uint64 value is 0. Can be used to explicitly indicate no-value.
static always_inline opaque_t opaque_null() {
  return u2o(0);
}

// Are the two given opaques identical.
static always_inline bool opaque_same(opaque_t a, opaque_t b) {
  return a.as_uint64 == b.as_uint64;
}

#endif // _OPAQUE_H
