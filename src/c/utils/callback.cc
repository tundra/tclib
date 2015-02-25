//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/stdc.h"
#include "utils/callback.hh"
#include "utils/alloc.hh"

using namespace tclib;

// Given a callback, creates a heap-allocated clone and casts it to the given C
// type.
template <typename C, typename T>
C *clone_and_cloak(const T &callback) {
  abstract_callback_t *clone = new (kDefaultAlloc) T(callback);
  return reinterpret_cast<C*>(clone);
}

template <typename T, typename C>
callback_t<T> &uncloak(C *cloaked) {
  return *reinterpret_cast<callback_t<T>*>(cloaked);
}

nullary_callback_t *nullary_callback_new_0(opaque_t (invoker)(void)) {
  return clone_and_cloak<nullary_callback_t>(new_callback(invoker));
}

nullary_callback_t *nullary_callback_new_1(opaque_t (invoker)(opaque_t), opaque_t b0) {
  return clone_and_cloak<nullary_callback_t>(new_callback(invoker, b0));
}

nullary_callback_t *nullary_callback_new_2(opaque_t (invoker)(opaque_t, opaque_t),
    opaque_t b0, opaque_t b1) {
  return clone_and_cloak<nullary_callback_t>(new_callback(invoker, b0, b1));
}

opaque_t nullary_callback_call(nullary_callback_t *callback) {
  return (uncloak<opaque_t(void)>(callback))();
}

unary_callback_t *unary_callback_new_0(opaque_t (invoker)(opaque_t)) {
  return clone_and_cloak<unary_callback_t>(new_callback(invoker));
}

unary_callback_t *unary_callback_new_1(opaque_t (invoker)(opaque_t, opaque_t),
    opaque_t b0) {
  return clone_and_cloak<unary_callback_t>(new_callback(invoker, b0));
}

unary_callback_t *unary_callback_new_2(opaque_t (invoker)(opaque_t, opaque_t, opaque_t),
    opaque_t b0, opaque_t b1) {
  return clone_and_cloak<unary_callback_t>(new_callback(invoker, b0, b1));
}

opaque_t unary_callback_call(unary_callback_t *callback, opaque_t a0) {
  return (uncloak<opaque_t(opaque_t)>(callback))(a0);
}

unary_callback_t *tclib::unary_callback_from(callback_t<opaque_t(opaque_t)> *callback) {
  return reinterpret_cast<unary_callback_t*>(callback);
}

void callback_destroy(void *raw_callback) {
  // This uses the fact that concrete callback types don't add state in addition
  // to what is in abstract callback.
  abstract_callback_t *callback = reinterpret_cast<abstract_callback_t*>(raw_callback);
  tclib::default_delete_concrete(callback);
}

void *callback_invisible_clone(void *raw_callback) {
  abstract_callback_t *callback = reinterpret_cast<abstract_callback_t*>(raw_callback);
  abstract_callback_t *clone = new abstract_callback_t(*callback);
  tclib::default_delete_concrete(callback);
  return clone;
}
