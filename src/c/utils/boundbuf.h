//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_BOUNDBUF_H
#define _TCLIB_BOUNDBUF_H

#include "c/stdc.h"
#include "utils/opaque.h"

// A circular bounded buffer that holds at most a fixed number of elements and
// can yield them in first-in first-out order. Not thread safe out of the box.
// Bounded buffers are generic on the size of the buffer (number of elements)
// as well as the width of each element, so use bounded_buffer_t(EC, EW) to
// refer to the type for a particular set of parameters.
typedef struct {
  // The max number of elements this buffer will hold.
  size_t capacity;
  // How many slots are occupied?
  size_t occupied_count;
  // Index of the next free slot.
  size_t next_free;
  // Index of the next occupied slot to take a value from.
  size_t next_occupied;
  // Number of opaques stored in each entry in the buffer.
  size_t element_width;
} generic_bounded_buffer_t;

// See the non-generic version below.
void generic_bounded_buffer_init(generic_bounded_buffer_t *generic,
    opaque_t *data, size_t capacity, size_t width);

// See the non-generic version below.
bool generic_bounded_buffer_is_empty(generic_bounded_buffer_t *generic);

// See the non-generic version below.
bool generic_bounded_buffer_try_offer(generic_bounded_buffer_t *generic,
    opaque_t *data, opaque_t *values, size_t elmw);

// See the non-generic version below.
bool generic_bounded_buffer_try_take(generic_bounded_buffer_t *generic,
    opaque_t *data, opaque_t *values_out, size_t elmw);

#define __BBNAME__(EC, EW, NAME) JOIN5(bounded_buffer, EC, by, EW, NAME)

// Generic bounded buffer type.
#define bounded_buffer_t(EC, EW) __BBNAME__(EC, EW, t)

// Expands to the the declaration of a bounded buffer of the given size.
#define DECLARE_BOUNDED_BUFFER(EC, EW)                                         \
typedef struct {                                                               \
  generic_bounded_buffer_t generic;                                            \
  opaque_t data[(EC) * (EW)];                                                  \
} bounded_buffer_t(EC, EW);                                                    \
void __BBNAME__(EC, EW, init)(bounded_buffer_t(EC, EW) *buf);                  \
bool __BBNAME__(EC, EW, is_empty)(bounded_buffer_t(EC, EW) *buf);              \
bool __BBNAME__(EC, EW, try_offer)(bounded_buffer_t(EC, EW) *buf,              \
    opaque_t *values, size_t elmw);                                            \
bool __BBNAME__(EC, EW, try_take)(bounded_buffer_t(EC, EW) *buf,               \
    opaque_t *values_out, size_t elmw)

// Expands to the implementation of a bounded buffer of the given size.
#define IMPLEMENT_BOUNDED_BUFFER(EC, EW)                                       \
void __BBNAME__(EC, EW, init)(bounded_buffer_t(EC, EW) *buf) {                 \
  generic_bounded_buffer_init(&buf->generic, buf->data, (EC), (EW));           \
}                                                                              \
bool __BBNAME__(EC, EW, is_empty)(bounded_buffer_t(EC, EW) *buf) {             \
  return generic_bounded_buffer_is_empty(&buf->generic);                       \
}                                                                              \
bool __BBNAME__(EC, EW, try_offer)(bounded_buffer_t(EC, EW) *buf,              \
    opaque_t *values, size_t elmw) {                                           \
  return generic_bounded_buffer_try_offer(&buf->generic, buf->data, values,    \
      elmw);                                                                   \
}                                                                              \
bool __BBNAME__(EC, EW, try_take)(bounded_buffer_t(EC, EW) *buf,               \
    opaque_t *values_out, size_t elmw) {                                       \
  return generic_bounded_buffer_try_take(&buf->generic, buf->data, values_out, \
      elmw);                                                                   \
}

// Initialize a bounded buffer that can hold up to 'EC' elements where each
// element is made up of 'EW' opaques.
#define bounded_buffer_init(EC, EW) __BBNAME__(EC, EW, init)

// Attempt to add a value in the next free slot of the given buffer. Returns
// true if successful.
#define bounded_buffer_try_offer(EC, EW) __BBNAME__(EC, EW, try_offer)

// Attempt to take a value from the next occupied slot in the given buffer.
// Returns true if successful, in which case the element will be stored in the
// out parameter.
#define bounded_buffer_try_take(EC, EW) __BBNAME__(EC, EW, try_take)

// Returns true if the given buffer has no elements.
#define bounded_buffer_is_empty(EC, EW) __BBNAME__(EC, EW, is_empty)

DECLARE_BOUNDED_BUFFER(16, 1);
DECLARE_BOUNDED_BUFFER(256, 1);

#endif // _TCLIB_BOUNDBUF_H
