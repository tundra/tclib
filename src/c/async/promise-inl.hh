//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PROMISE_INL_HH
#define _TCLIB_PROMISE_INL_HH

#include "async/promise.hh"
#include "sync/semaphore.hh"

namespace tclib {

template <typename T, typename E>
promise_state_t<T, E>::Locker::Locker(promise_state_t<T, E> *state)
  : state_(state) {
  state_->lock();
}

template <typename T, typename E>
promise_state_t<T, E>::Locker::~Locker() {
  state_->unlock();
}

template <typename T, typename E>
const T &promise_state_t<T, E>::unsafe_get_value() {
  return *pointers_.as_value;
}

template <typename T, typename E>
const E &promise_state_t<T, E>::unsafe_get_error() {
  return *pointers_.as_error;
}

template <typename T, typename E>
bool promise_state_t<T, E>::is_resolved() {
  Locker lock(this);
  return state_ != psEmpty;
}

template <typename T, typename E>
bool promise_state_t<T, E>::has_succeeded() {
  Locker lock(this);
  return state_ == psSucceeded;
}

template <typename T, typename E>
bool promise_state_t<T, E>::has_failed() {
  Locker lock(this);
  return state_ == psFailed;
}

template <typename T, typename E>
void promise_state_t<T, E>::unsafe_set_value(const T &value) {
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
void promise_state_t<T, E>::unsafe_set_error(const E &error) {
  // See the massive comment in set_value.
  pointers_.as_error = new (memory_.as_error) E(error);
}

template <typename T, typename E>
promise_state_t<T, E>::promise_state_t()
  : state_(psEmpty) {
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
    unsafe_get_value().~T();
  } else if (state_ == psFailed) {
    unsafe_get_error().~E();
  }
}

template <typename T, typename E>
bool promise_state_t<T, E>::fulfill(const T &value) {
  {
    Locker lock(this);
    if (state_ != psEmpty)
      return false;
    // Set the value before the state such that it is safe to assume the value
    // is set when the state is non-empty. This is used by peek_value.
    unsafe_set_value(value);
    state_ = psSucceeded;
  }
  // At this point any new on_success added will be executed immediately so we
  // have the vector to ourselves.
  for (size_t i = 0; i < on_successes_.size(); i++)
    (on_successes_[i])(value);
  on_successes_.clear();
  return true;
}

template <typename T, typename E>
bool promise_state_t<T, E>::fail(const E &error) {
  {
    Locker lock(this);
    if (state_ != psEmpty)
      return false;
    // See fulfill for why we set the error first.
    unsafe_set_error(error);
    state_ = psFailed;
  }
  // At this point any new on_failure added will be executed immediately so we
  // have the vector to ourselves.
  for (size_t i = 0; i < on_failures_.size(); i++)
    (on_failures_[i])(error);
  on_failures_.clear();
  return true;
}

template <typename T, typename E>
const T &promise_state_t<T, E>::peek_value(const T &if_unfulfilled) {
  // Because the value is set before state_ is changed we know that it's safe
  // to read the value in the success case even without taking the mutex.
  return (state_ == psSucceeded) ? unsafe_get_value() : if_unfulfilled;
}

template <typename T, typename E>
const E &promise_state_t<T, E>::peek_error(const E &if_unfulfilled) {
  // See peek_value for why this doesn't need to lock.
  return (state_ == psFailed) ? unsafe_get_error() : if_unfulfilled;
}

template <typename T, typename E>
void promise_state_t<T, E>::on_success(SuccessAction action) {
  {
    Locker lock(this);
    if (state_ == psEmpty) {
      on_successes_.push_back(action);
      return;
    }
  }
  // We know the promise has been fulfilled here so no need to lock.
  if (state_ == psSucceeded)
    action(unsafe_get_value());
}

template <typename T, typename E>
void promise_state_t<T, E>::on_failure(FailureAction action) {
  {
    Locker lock(this);
    if (state_ == psEmpty) {
      on_failures_.push_back(action);
      return;
    }
  }
  // We know the promise has been fulfilled here so no need to lock.
  if (state_ == psFailed)
    action(unsafe_get_error());
}

template <typename T, typename E>
template <typename T2, typename E2>
void promise_t<T, E>::map_and_fulfill(promise_t<T2, E2> dest, callback_t<T2(T)> mapper,
    T value) {
  dest.fulfill(mapper(value));
}

template <typename T, typename E>
template <typename T2, typename E2>
void promise_t<T, E>::map_and_fail(promise_t<T2, E2> dest, callback_t<E2(E)> mapper,
    E error) {
  dest.fail(mapper(error));
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
  on_success(new_callback(map_and_fulfill<T2, E>, result, mapper));
  on_failure(new_callback(pass_on_failure<T2>, result));
  return result;
}

template <typename T, typename E>
template <typename T2, typename E2>
promise_t<T2, E2> promise_t<T, E>::then(callback_t<T2(T)> vmap, callback_t<E2(E)> emap) {
  promise_t<T2, E2> result = promise_t<T2, E2>::empty();
  on_success(new_callback(map_and_fulfill<T2, E2>, result, vmap));
  on_failure(new_callback(map_and_fail<T2, E2>, result, emap));
  return result;
}

template <typename T, typename E>
promise_t<T, E> promise_t<T, E>::empty() {
  return promise_t<T, E>(new (kDefaultAlloc) promise_state_t<T, E>());
}

template <typename T, typename E>
sync_promise_t<T, E> sync_promise_t<T, E>::empty() {
  return sync_promise_t<T, E>(new (kDefaultAlloc) sync_promise_state_t<T, E>());
}

template <typename T, typename E>
template <typename I>
void sync_promise_state_t<T, E>::lower_drawbridge(Drawbridge *drawbridge, I) {
  drawbridge->lower();
}

template <typename T, typename E>
bool sync_promise_state_t<T, E>::wait(Duration timeout) {
  return drawbridge_.pass(timeout);
}

template <typename T, typename E>
sync_promise_state_t<T, E>::sync_promise_state_t() {
  drawbridge_.initialize();
  this->on_successes_.push_back(new_callback(lower_drawbridge<T>, &drawbridge_));
  this->on_failures_.push_back(new_callback(lower_drawbridge<E>, &drawbridge_));
}

} // namespace tclib

#endif // _TCLIB_PROMISE_INL_HH
