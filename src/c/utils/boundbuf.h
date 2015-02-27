//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_BOUNDBUF_H
#define _TCLIB_BOUNDBUF_H

#include "c/stdc.h"
#include "utils/opaque.h"

// A circular bounded buffer that holds at most a fixed number of elements and
// can yield them in first-in first-out order. Not thread safe out of the box.
typedef struct {
  // The max number of elements this buffer will hold.
  size_t capacity;
  // How many slots are occupied?
  size_t occupied_count;
  // Index of the next free slot.
  size_t next_free;
  // Index of the next occupied slot to take a value from.
  size_t next_occupied;
  // Beginning of the memory that holds the values.
  opaque_t data[1];
} bounded_buffer_t;

// Yields the size in bytes of a bounded buffer with the given number of
// elements.
#define BOUNDED_BUFFER_SIZE(N) sizeof(bounded_buffer_t) + sizeof(opaque_t) * ((N) - 1)

// Initialize a bounded buffer that can hold up to the given number of elements.
// The size is the size of the given buffer, it is used to sanity check whether
// it's large enough.
void bounded_buffer_init(void *raw_buf, size_t size, size_t capacity);

// Attempt to add a value in the next free slot of the given buffer. Returns
// true if successful.
bool bounded_buffer_try_offer(void *raw_buf, opaque_t value);

// Attempt to take a value from the next occupied slot in the given buffer.
// Returns true if successful, in which case the element will be stored in the
// out parameter.
bool bounded_buffer_try_take(void *raw_buf, opaque_t *value_out);

// Returns true if the given buffer has no elements.
bool bounded_buffer_is_empty(void *raw_buf);

#endif // _TCLIB_BOUNDBUF_H
