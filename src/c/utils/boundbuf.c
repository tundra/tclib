//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "boundbuf.h"
#include "log.h"

void generic_bounded_buffer_init(generic_bounded_buffer_t *generic,
    opaque_t *data, size_t capacity, size_t width) {
  CHECK_TRUE("zero-width boundbuf", width > 0);
  memset(data, 0, capacity * width * sizeof(opaque_t));
  generic->capacity = capacity;
  generic->occupied_count = 0;
  generic->next_free = 0;
  generic->next_occupied = 0;
  generic->element_width = width;
}

bool generic_bounded_buffer_is_empty(generic_bounded_buffer_t *generic) {
  return generic->occupied_count == 0;
}

bool generic_bounded_buffer_try_offer(generic_bounded_buffer_t *generic,
    opaque_t *data, opaque_t *values, size_t elmw) {
  CHECK_EQ("unaligned boundbuf offer", elmw, generic->element_width);
  if (generic->occupied_count == generic->capacity)
    return false;
  opaque_t *slot = &data[generic->next_free * generic->element_width];
  CHECK_TRUE("overwriting", opaque_is_null(*slot));
  memcpy(slot, values, sizeof(opaque_t) * generic->element_width);
  generic->next_free = (generic->next_free + 1) % generic->capacity;
  generic->occupied_count++;
  return true;
}

bool generic_bounded_buffer_try_take(generic_bounded_buffer_t *generic,
    opaque_t *data, opaque_t *values_out, size_t elmw) {
  CHECK_EQ("unaligned boundbuf take", elmw, generic->element_width);
  if (generic->occupied_count == 0)
    return false;
  opaque_t *slot = &data[generic->next_occupied * generic->element_width];
  memcpy(values_out, slot, sizeof(opaque_t) * generic->element_width);
  memset(slot, 0, sizeof(opaque_t) * generic->element_width);
  generic->next_occupied = (generic->next_occupied + 1) % generic->capacity;
  generic->occupied_count--;
  return true;
}

IMPLEMENT_BOUNDED_BUFFER(16, 1)
