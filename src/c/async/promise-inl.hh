//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PROMISE_INL_HH
#define _TCLIB_PROMISE_INL_HH

#include "async/promise.hh"

namespace tclib {

template <typename T, typename E>
promise_state_t<T, E>::promise_state_t()
  : refcount_(0)
  , state_(psEmpty) {
  // These two overlap so really there is some redundancy here but the compiler
  // can probably figure it out, if not it doesn't matter.
  memset(data_.as_value, 0, sizeof(T));
  memset(data_.as_error, 0, sizeof(E));
}

template <typename T, typename E>
promise_state_t<T, E>::~promise_state_t() {
  if (state_ == psSucceeded) {
    as_value().~T();
  } else if (state_ == psFailed) {
    as_error().~E();
  }
}

template <typename T, typename E>
bool promise_state_t<T, E>::fulfill(const T &value) {
  if (state_ != psEmpty)
    return false;
  state_ = psSucceeded;
  as_value() = value;
  for (size_t i = 0; i < on_successes_.size(); i++)
    (on_successes_[i])(value);
  on_successes_.clear();
  return true;
}

template <typename T, typename E>
bool promise_state_t<T, E>::fail(const E &value) {
  if (state_ != psEmpty)
    return false;
  state_ = psFailed;
  as_error() = value;
  for (size_t i = 0; i < on_failures_.size(); i++)
    (on_failures_[i])(value);
  on_failures_.clear();
  return true;
}

template <typename T, typename E>
T promise_state_t<T, E>::get_value(T if_unfulfilled) {
  return (state_ == psSucceeded) ? as_value() : if_unfulfilled;
}

template <typename T, typename E>
E promise_state_t<T, E>::get_error(E if_unfulfilled) {
  return (state_ == psFailed) ? as_error() : if_unfulfilled;
}

template <typename T, typename E>
void promise_state_t<T, E>::on_success(SuccessAction action) {
  if (state_ == psEmpty) {
    on_successes_.push_back(action);
  } else if (state_ == psSucceeded) {
    action(as_value());
  }
}

template <typename T, typename E>
void promise_state_t<T, E>::on_failure(FailureAction action) {
  if (state_ == psEmpty) {
    on_failures_.push_back(action);
  } else if (state_ == psFailed) {
    action(as_error());
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
