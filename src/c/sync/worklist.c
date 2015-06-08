//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "utils/log.h"
#include "sync/worklist.h"

bool generic_worklist_init(generic_worklist_t *generic,
    generic_bounded_buffer_t *generic_buffer, opaque_t *buffer_data,
    size_t capacity, size_t width) {
  generic_bounded_buffer_init(generic_buffer, buffer_data, capacity, width);
  native_mutex_construct(&generic->guard);
  native_semaphore_construct_with_count(&generic->vacancies, (uint32_t) capacity);
  native_semaphore_construct_with_count(&generic->occupied, 0);
  return native_mutex_initialize(&generic->guard)
      && native_semaphore_initialize(&generic->vacancies)
      && native_semaphore_initialize(&generic->occupied);
}

bool generic_worklist_schedule(generic_worklist_t *generic,
    generic_bounded_buffer_t *generic_buffer, opaque_t *buffer_data,
    opaque_t *values, size_t elmw, duration_t timeout) {
  if (!native_semaphore_acquire(&generic->vacancies, timeout)
      || !native_mutex_lock(&generic->guard))
    return false;
  bool offered = generic_bounded_buffer_try_offer(generic_buffer, buffer_data,
      values, elmw);
  CHECK_TRUE("failed to offer", offered);
  return native_semaphore_release(&generic->occupied)
      && native_mutex_unlock(&generic->guard);
}

bool generic_worklist_is_empty(generic_bounded_buffer_t *generic_buffer) {
  return generic_bounded_buffer_is_empty(generic_buffer);
}

bool generic_worklist_take(generic_worklist_t *generic,
    generic_bounded_buffer_t *generic_buffer, opaque_t *buffer_data,
    opaque_t *values_out, size_t elmw, duration_t timeout) {
  if (!native_semaphore_acquire(&generic->occupied, timeout)
      || !native_mutex_lock(&generic->guard))
    return false;
  bool took = generic_bounded_buffer_try_take(generic_buffer, buffer_data,
      values_out, elmw);
  CHECK_TRUE("failed to take", took);
  return native_semaphore_release(&generic->vacancies)
      && native_mutex_unlock(&generic->guard);
}

void generic_worklist_dispose(generic_worklist_t *generic) {
  native_mutex_dispose(&generic->guard);
  native_semaphore_dispose(&generic->vacancies);
}

IMPLEMENT_WORKLIST(16, 1)
IMPLEMENT_WORKLIST(256, 1)
