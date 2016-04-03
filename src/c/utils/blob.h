//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_BLOB_H
#define _TCLIB_BLOB_H

#include "c/stdc.h"

#include "sync/atomic.h"
#include "utils/check.h"

// A block of memory as returned from an allocator. Bundling the length with the
// memory allows us to check how much memory is live at any given time.
typedef struct {
  // The actual memory.
  void *start;
  // The number of bytes of memory (minus any overhead added by the allocator).
  size_t size;
} blob_t;

// Creates a new memory block with the given contents. Be sure to note that this
// doesn't allocate anything, just bundles previously allocated memory into a
// struct.
static inline blob_t blob_new(void *start, size_t size) {
  blob_t result;
  result.start = start;
  result.size = size;
  return result;
}

// Returns true iff the given block is empty, say because allocation failed.
static inline bool blob_is_empty(blob_t block) {
  return block.start == NULL;
}

// Resets the given block to the empty state.
static inline blob_t blob_empty() {
  return blob_new(NULL, 0);
}

// Fills this memory block's data with the given value.
static inline void blob_fill(blob_t block, byte_t value) {
  memset(block.start, value, block.size);
}

// Write the contents of the source blob into the destination.
static inline void blob_copy_to(blob_t src, blob_t dest) {
  CHECK_REL("blob copy destination too small", dest.size, >=, src.size);
  memcpy(dest.start, src.start, src.size);
}

// Returns true if both blobs have the same contents.
bool blob_equals(blob_t a, blob_t b);

// Given a struct by value, fills the memory occupied by the struct with zeroes.
#define struct_zero_fill(V) blob_fill(blob_new(&(V), sizeof(V)), 0)

#endif // _TCLIB_BLOB_H
