//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/winhdr.h"

int32_t atomic_int32_increment(atomic_int32_t *value) {
  return InterlockedIncrement(&value->value);
}

int32_t atomic_int32_decrement(atomic_int32_t *value) {
  return InterlockedDecrement(&value->value);
}
