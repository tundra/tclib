//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "utils/callback.h"
END_C_INCLUDES

#include "utils/callback.hh"
using namespace tclib;

// Given a callback, creates a heap-allocated clone and casts it to the given C
// type.
template <typename C, typename T>
C *clone_and_cloak(const T &callback) {
  abstract_callback_t *clone = new T(callback);
  return reinterpret_cast<C*>(clone);
}

template <typename T, typename C>
callback_t<T> &uncloak(C *cloaked) {
  return *reinterpret_cast<callback_t<T>*>(cloaked);
}

voidp_callback_t *voidp_callback_0(opaque_t (invoker)(void)) {
  return clone_and_cloak<voidp_callback_t>(new_callback(invoker));
}

voidp_callback_t *voidp_callback_voidp_1(opaque_t (invoker)(opaque_t), opaque_t b0) {
  return clone_and_cloak<voidp_callback_t>(new_callback(invoker, b0));
}

opaque_t voidp_callback_call(voidp_callback_t *callback) {
  return (uncloak<opaque_t(void)>(callback))();
}

voidp_callback_voidp_t *voidp_callback_voidp_0(opaque_t (invoker)(opaque_t)) {
  return clone_and_cloak<voidp_callback_voidp_t>(new_callback(invoker));
}

opaque_t voidp_callback_voidp_call(voidp_callback_voidp_t *callback, opaque_t a0) {
  return (uncloak<opaque_t(opaque_t)>(callback))(a0);
}

void callback_dispose(void *raw_callback) {
  abstract_callback_t *callback = reinterpret_cast<abstract_callback_t*>(raw_callback);
  delete callback;
}
