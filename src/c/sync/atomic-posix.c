//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

int32_t atomic_int32_increment(atomic_int32_t *value) {
  return __atomic_add_fetch(&value->value, 1, __ATOMIC_RELAXED);
}

int32_t atomic_int32_decrement(atomic_int32_t *value) {
  return __atomic_add_fetch(&value->value, -1, __ATOMIC_RELAXED);
}
