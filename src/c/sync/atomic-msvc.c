//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

int32_t atomic_int32_increment(atomic_int32_t *value) {
  return atomic_int32_add(value, 1);
}

int32_t atomic_int32_decrement(atomic_int32_t *value) {
  return atomic_int32_add(value, -1);
}

int32_t atomic_int32_add(atomic_int32_t *value, int32_t delta) {
  // It'd be great if we could use InterlockedAdd but it's not available on
  // 32-bit.
  return InterlockedExchangeAdd(&value->value, delta) + delta;
}

int32_t atomic_int32_subtract(atomic_int32_t *value, int32_t delta) {
  return atomic_int32_add(value, -delta);
}

bool atomic_int32_compare_and_set(atomic_int32_t *value, int32_t old_value,
    int32_t new_value) {
  return old_value == InterlockedCompareExchange(
      &value->value, // Destination
      new_value, // Exchange
      old_value); // Comparand
}

int64_t atomic_int64_increment(atomic_int64_t *value) {
  return atomic_int64_add(value, 1);
}

int64_t atomic_int64_decrement(atomic_int64_t *value) {
  return atomic_int64_add(value, -1);
}

int64_t atomic_int64_add(atomic_int64_t *value, int64_t delta) {
  return InterlockedExchangeAdd64(&value->value, delta) + delta;
}

int64_t atomic_int64_subtract(atomic_int64_t *value, int64_t delta) {
  return atomic_int64_add(value, -delta);
}
