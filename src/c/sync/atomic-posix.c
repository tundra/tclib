//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// The name of the add-and-fetch builtin changed in GCC 4.7.
#define add_and_fetch_builtin IF_GCC_4_7(__atomic_add_fetch, __sync_add_and_fetch)

int32_t atomic_int32_increment(atomic_int32_t *value) {
  return add_and_fetch_builtin(&value->value, 1, __ATOMIC_RELAXED);
}

int32_t atomic_int32_decrement(atomic_int32_t *value) {
  return add_and_fetch_builtin(&value->value, -1, __ATOMIC_RELAXED);
}
