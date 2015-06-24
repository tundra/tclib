//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "utils/check.h"
#include "utils/lifetime.h"

static lifetime_t *default_lifetime = NULL;

bool lifetime_begin(lifetime_t *lifetime) {
  voidp_vector_init(&lifetime->callbacks);
  native_mutex_construct(&lifetime->guard);
  return native_mutex_initialize(&lifetime->guard);
}

void lifetime_atexit(lifetime_t *lifetime, nullary_callback_t *callback) {
  voidp_vector_push_back(&lifetime->callbacks, callback);
}

void lifetime_end(lifetime_t *lifetime) {
  size_t count = voidp_vector_size(&lifetime->callbacks);
  for (size_t i = 0; i < count; i++) {
    nullary_callback_t *callback = (nullary_callback_t*) voidp_vector_get(&lifetime->callbacks, i);
    nullary_callback_call(callback);
    callback_destroy(callback);
  }
  voidp_vector_dispose(&lifetime->callbacks);
  native_mutex_dispose(&lifetime->guard);
}

bool lifetime_begin_default(lifetime_t *lifetime) {
  CHECK_TRUE("multiple default lifetime", default_lifetime == NULL);
  default_lifetime = lifetime;
  return lifetime_begin(lifetime);
}

void lifetime_end_default(lifetime_t *lifetime) {
  CHECK_TRUE("wrong default lifetime", default_lifetime == lifetime);
  default_lifetime = NULL;
  lifetime_end(lifetime);
}

lifetime_t *lifetime_get_default() {
  return default_lifetime;
}
