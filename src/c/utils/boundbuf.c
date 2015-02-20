//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "boundbuf.h"
#include "log.h"

void bounded_buffer_init(bounded_buffer_t *buf, size_t capacity) {
  memset(buf, 0, BOUNDED_BUFFER_SIZE(capacity));
  buf->capacity = capacity;
  buf->occupied_count = 0;
  buf->next_free = 0;
  buf->next_occupied = 0;
}

bool bounded_buffer_try_offer(bounded_buffer_t *buf, opaque_t value) {
  if (buf->occupied_count == buf->capacity)
    return false;
  opaque_t *slot = &buf->data[buf->next_free];
  CHECK_TRUE("overwriting", opaque_is_null(*slot));
  *slot = value;
  buf->next_free = (buf->next_free + 1) % buf->capacity;
  buf->occupied_count++;
  return true;
}

bool bounded_buffer_try_take(bounded_buffer_t *buf, opaque_t *value_out) {
  if (buf->occupied_count == 0)
    return false;
  *value_out = buf->data[buf->next_occupied];
  buf->data[buf->next_occupied] = opaque_null();
  buf->next_occupied = (buf->next_occupied + 1) % buf->capacity;
  buf->occupied_count--;
  return true;
}
