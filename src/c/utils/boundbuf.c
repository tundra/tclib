//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "boundbuf.h"
#include "log.h"

void generic_bounded_buffer_init(generic_bounded_buffer_t *generic,
    opaque_t *data, size_t capacity) {
  memset(data, 0, capacity * sizeof(opaque_t));
  generic->capacity = capacity;
  generic->occupied_count = 0;
  generic->next_free = 0;
  generic->next_occupied = 0;
}

bool generic_bounded_buffer_is_empty(generic_bounded_buffer_t *generic) {
  return generic->occupied_count == 0;
}

bool generic_bounded_buffer_try_offer(generic_bounded_buffer_t *generic,
    opaque_t *data, size_t capacity, opaque_t value) {
  if (generic->occupied_count == generic->capacity)
    return false;
  opaque_t *slot = &data[generic->next_free];
  CHECK_TRUE("overwriting", opaque_is_null(*slot));
  *slot = value;
  generic->next_free = (generic->next_free + 1) % generic->capacity;
  generic->occupied_count++;
  return true;
}

bool generic_bounded_buffer_try_take(generic_bounded_buffer_t *generic,
    opaque_t *data, size_t capacity, opaque_t *value_out) {
  if (generic->occupied_count == 0)
    return false;
  *value_out = data[generic->next_occupied];
  data[generic->next_occupied] = opaque_null();
  generic->next_occupied = (generic->next_occupied + 1) % generic->capacity;
  generic->occupied_count--;
  return true;
}

IMPLEMENT_BOUNDED_BUFFER(16)
