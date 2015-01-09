//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PROMISE_INL_HH
#define _TCLIB_PROMISE_INL_HH

#include "async/promise.hh"

namespace tclib {

template <typename T, typename E>
const T &promise_state_t<T, E>::get_value() {
  return *pointers_.as_value;
}

template <typename T, typename E>
const E &promise_state_t<T, E>::get_error() {
  return *pointers_.as_error;
}

template <typename T, typename E>
void promise_state_t<T, E>::set_value(const T &value) {
  // This is tricky but if you think about it long enough it turns out to do
  // exactly what you want. What you want is to not have to construct the
  // initial value and error; it's pointless work and requires them to have
  // no-arg constructors for no particularly good reason. Instead we keep blank
  // memory and initialize it when the promise is fulfilled. You could in
  // principle use an assignment for that here -- cast the memory to T and then
  // assign the value -- but that may cause operator= to be called where the
  // thing being assigned has not been initialized (because it's just blank
  // memory at this point). So instead we call the copy constructor on the
  // memory, using placement new. That requires no assumptions about the
  // contents of the memory, and it conveniently gives us back a pointer of the
  // right type which we keep for later use; strict aliasing makes it really
  // tricky to get this otherwise.
  pointers_.as_value = new (memory_.as_value) T(value);
}

template <typename T, typename E>
void promise_state_t<T, E>::set_error(const E &error) {
  // See the massive comment in set_value.
  pointers_.as_error = new (memory_.as_error) E(error);
}

template <typename T, typename E>
promise_state_t<T, E>::promise_state_t()
  : refcount_(0)
  , state_(psEmpty) {
  // These two overlap so really there is some redundancy here but the compiler
  // can probably figure it out, if not it doesn't matter.
  memset(memory_.as_value, 0, sizeof(T));
  memset(memory_.as_error, 0, sizeof(E));
}

template <typename T, typename E>
promise_state_t<T, E>::~promise_state_t() {
  // Because of the initialization trickery we'll only know dynamically whether
  // the value or error have been initialized. Hence we need to call the
  // destructor explicitly.
  if (state_ == psSucceeded) {
    get_value().~T();
  } else if (state_ == psFailed) {
    get_error().~E();
  }
}

template <typename T, typename E>
bool promise_state_t<T, E>::fulfill(const T &value) {
  if (state_ != psEmpty)
    return false;
  state_ = psSucceeded;
  set_value(value);
  for (size_t i = 0; i < on_successes_.size(); i++)
    (on_successes_[i])(value);
  on_successes_.clear();
  return true;
}

template <typename T, typename E>
bool promise_state_t<T, E>::fail(const E &error) {
  if (state_ != psEmpty)
    return false;
  state_ = psFailed;
  set_error(error);
  for (size_t i = 0; i < on_failures_.size(); i++)
    (on_failures_[i])(error);
  on_failures_.clear();
  return true;
}

template <typename T, typename E>
const T &promise_state_t<T, E>::get_value(const T &if_unfulfilled) {
  return (state_ == psSucceeded) ? get_value() : if_unfulfilled;
}

template <typename T, typename E>
const E &promise_state_t<T, E>::get_error(const E &if_unfulfilled) {
  return (state_ == psFailed) ? get_error() : if_unfulfilled;
}

template <typename T, typename E>
void promise_state_t<T, E>::on_success(SuccessAction action) {
  if (state_ == psEmpty) {
    on_successes_.push_back(action);
  } else if (state_ == psSucceeded) {
    action(get_value());
  }
}

template <typename T, typename E>
void promise_state_t<T, E>::on_failure(FailureAction action) {
  if (state_ == psEmpty) {
    on_failures_.push_back(action);
  } else if (state_ == psFailed) {
    action(get_error());
  }
}

template <typename T, typename E>
template <typename T2>
void promise_t<T, E>::map_and_fulfill(promise_t<T2, E> dest, callback_t<T2(T)> mapper,
    T value) {
  dest.fulfill(mapper(value));
}

template <typename T, typename E>
template <typename T2>
void promise_t<T, E>::pass_on_failure(promise_t<T2, E> dest, E error) {
  dest.fail(error);
}

template <typename T, typename E>
template <typename T2>
promise_t<T2, E> promise_t<T, E>::then(callback_t<T2(T)> mapper) {
  promise_t<T2, E> result = promise_t<T2, E>::empty();
  on_success(new_callback(map_and_fulfill<T2>, result, mapper));
  on_failure(new_callback(pass_on_failure<T2>, result));
  return result;
}

template <typename T, typename E>
promise_t<T, E> promise_t<T, E>::empty() {
  return promise_t<T, E>(new promise_state_t<T, E>());
}

} // namespace tclib

#endif // _TCLIB_PROMISE_INL_HH
