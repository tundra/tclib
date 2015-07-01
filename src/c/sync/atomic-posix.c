//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

int32_t atomic_int32_increment(atomic_int32_t *value) {
  return atomic_int32_add(value, 1);
}

int32_t atomic_int32_decrement(atomic_int32_t *value) {
  return atomic_int32_add(value, -1);
}

int32_t atomic_int32_add(atomic_int32_t *value, int32_t delta) {
  return IF_GCC_4_7(
      __atomic_add_fetch(&value->value, delta, __ATOMIC_RELAXED),
      __sync_add_and_fetch(&value->value, delta));
}

int32_t atomic_int32_subtract(atomic_int32_t *value, int32_t delta) {
  return atomic_int32_add(value, -delta);
}

int64_t atomic_int64_increment(atomic_int64_t *value) {
  return atomic_int64_add(value, 1);
}

int64_t atomic_int64_decrement(atomic_int64_t *value) {
  return atomic_int64_add(value, -1);
}

int64_t atomic_int64_add(atomic_int64_t *value, int64_t delta) {
  return IF_GCC_4_7(
      __atomic_add_fetch(&value->value, delta, __ATOMIC_RELAXED),
      __sync_add_and_fetch(&value->value, delta));
}

int64_t atomic_int64_subtract(atomic_int64_t *value, int64_t delta) {
  return atomic_int64_add(value, -delta);
}
