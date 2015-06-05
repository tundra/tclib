//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_BOUNDBUF_H
#define _TCLIB_BOUNDBUF_H

#include "c/stdc.h"
#include "utils/opaque.h"

// A circular bounded buffer that holds at most a fixed number of elements and
// can yield them in first-in first-out order. Not thread safe out of the box.
// Bounded buffers are generic on the buffer size so use bounded_buffer_t(N)
// to refer to the type for a particular size.
typedef struct {
  // The max number of elements this buffer will hold.
  size_t capacity;
  // How many slots are occupied?
  size_t occupied_count;
  // Index of the next free slot.
  size_t next_free;
  // Index of the next occupied slot to take a value from.
  size_t next_occupied;
} generic_bounded_buffer_t;

// See the non-generic version below.
void generic_bounded_buffer_init(generic_bounded_buffer_t *generic,
    opaque_t *data, size_t capacity);

// See the non-generic version below.
bool generic_bounded_buffer_is_empty(generic_bounded_buffer_t *generic);

// See the non-generic version below.
bool generic_bounded_buffer_try_offer(generic_bounded_buffer_t *generic,
    opaque_t *data, size_t capacity, opaque_t value);

// See the non-generic version below.
bool generic_bounded_buffer_try_take(generic_bounded_buffer_t *generic,
    opaque_t *data, size_t capacity, opaque_t *value_out);

// Generic bounded buffer type.
#define bounded_buffer_t(N) JOIN3(bounded_buffer, N, t)

// Expands to the the declaration of a bounded buffer of the given size.
#define DECLARE_BOUNDED_BUFFER(N)                                              \
typedef struct {                                                               \
  generic_bounded_buffer_t generic;                                            \
  opaque_t data[(N)];                                                          \
} JOIN3(bounded_buffer, N, t);                                                 \
void JOIN3(bounded_buffer, N, init)(bounded_buffer_t(N) *buf);                 \
bool JOIN3(bounded_buffer, N, is_empty)(bounded_buffer_t(N) *buf);             \
bool JOIN3(bounded_buffer, N, try_offer)(bounded_buffer_t(N) *buf, opaque_t value); \
bool JOIN3(bounded_buffer, N, try_take)(bounded_buffer_t(N) *buf, opaque_t *value_out)

// Expands to the implementation of a bounded buffer of the given size.
#define IMPLEMENT_BOUNDED_BUFFER(N)                                            \
void JOIN3(bounded_buffer, N, init)(bounded_buffer_t(N) *buf) {                \
  generic_bounded_buffer_init(&buf->generic, buf->data, (N));                  \
}                                                                              \
bool JOIN3(bounded_buffer, N, is_empty)(bounded_buffer_t(N) *buf) {            \
  return generic_bounded_buffer_is_empty(&buf->generic);                       \
}                                                                              \
bool JOIN3(bounded_buffer, N, try_offer)(bounded_buffer_t(N) *buf, opaque_t value) { \
  return generic_bounded_buffer_try_offer(&buf->generic, buf->data, (N), value);\
}                                                                              \
bool JOIN3(bounded_buffer, N, try_take)(bounded_buffer_t(N) *buf, opaque_t *value_out) { \
  return generic_bounded_buffer_try_take(&buf->generic, buf->data, (N), value_out); \
}

// Initialize a bounded buffer that can hold up to the given number of elements.
// The size is the size of the given buffer, it is used to sanity check whether
// it's large enough.
#define bounded_buffer_init(N) JOIN3(bounded_buffer, N, init)

// Attempt to add a value in the next free slot of the given buffer. Returns
// true if successful.
#define bounded_buffer_try_offer(N) JOIN3(bounded_buffer, N, try_offer)

// Attempt to take a value from the next occupied slot in the given buffer.
// Returns true if successful, in which case the element will be stored in the
// out parameter.
#define bounded_buffer_try_take(N) JOIN3(bounded_buffer, N, try_take)

// Returns true if the given buffer has no elements.
#define bounded_buffer_is_empty(N) JOIN3(bounded_buffer, N, is_empty)

DECLARE_BOUNDED_BUFFER(16);

#endif // _TCLIB_BOUNDBUF_H
