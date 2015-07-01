//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

int32_t atomic_int32_increment(atomic_int32_t *value) {
  return InterlockedIncrement(&value->value);
}

int32_t atomic_int32_decrement(atomic_int32_t *value) {
  return InterlockedDecrement(&value->value);
}

int32_t atomic_int32_add(atomic_int32_t *value, int32_t delta) {
  return InterlockedAdd(&value->value, delta);
}

int32_t atomic_int32_subtract(atomic_int32_t *value, int32_t delta) {
  return InterlockedAdd(&value->value, -delta);
}

int64_t atomic_int64_increment(atomic_int64_t *value) {
  return InterlockedIncrement64(&value->value);
}

int64_t atomic_int64_decrement(atomic_int64_t *value) {
  return InterlockedDecrement64(&value->value);
}

int64_t atomic_int64_add(atomic_int64_t *value, int64_t delta) {
  return InterlockedAdd64(&value->value, delta);
}

int64_t atomic_int64_subtract(atomic_int64_t *value, int64_t delta) {
  return InterlockedAdd64(&value->value, -delta);
}
