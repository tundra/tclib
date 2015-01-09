//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PROMISE_HH
#define _TCLIB_PROMISE_HH

#include "stdc.h"
#include "callback.hh"
#include "std/stdvector.hh"

namespace tclib {

template <typename T, typename E = void*>
class promise_state_t {
public:
  typedef callback_t<void(T)> SuccessAction;
  typedef callback_t<void(E)> FailureAction;
  promise_state_t();
  ~promise_state_t();
  bool fulfill(const T &value);
  bool fail(const E &value);
  bool is_empty() { return state_ == psEmpty; }
  T get_value(T if_unfulfilled);
  E get_error(E if_unfulfilled);
  void on_success(SuccessAction action);
  void on_failure(FailureAction action);
  void ref();
  void deref();
private:
  typedef enum {
    psEmpty,
    psSucceeded,
    psFailed
  } state_t;
  size_t refcount_;
  state_t state_;
  std::vector<SuccessAction> on_successes_;
  std::vector<FailureAction> on_failures_;
  T &get_value();
  E &get_error();
  void set_value(const T &value);
  void set_error(const E &error);
  union {
    uint8_t as_value[sizeof(T)];
    uint8_t as_error[sizeof(E)];
  } data_;
};

template <typename T, typename E>
void promise_state_t<T, E>::ref() {
  refcount_++;
}

template <typename T, typename E>
void promise_state_t<T, E>::deref() {
  if (--refcount_ == 0)
    delete this;
}

// The result of an operation that may or may not have completed.
//
// There is always a heap allocated component to a promise but to avoid having
// to explicitly keep track of ownership of that component the heap state is
// allocated implicitly and refcounted so the code that passes around promises
// don't have to worry about ownership, which would be unmanageable.
template <typename T, typename E = void*>
class promise_t {
public:
  ~promise_t() { state_->deref(); }

  // Fulfills this promise, causing the value to be set and any actions to be
  // performed, but only if this promise is currently empty. Returns true iff
  // that was the case.
  bool fulfill(const T &value) {
    return state_->fulfill(value);
  }

  // Fails this promise, causing the error to be set and any failure actions to
  // be performed, but only if this promise is currently empty. Returns true iff
  // that was the case.
  bool fail(const E &error) {
    return state_->fail(error);
  }

  // Returns true iff this promise doesn't have its value set yet.
  bool is_empty() { return state_->is_empty(); }

  // Returns the value this promise resolved to or, if it hasn't been resolved
  // yet, the given default value.
  T get_value(T otherwise) { return state_->get_value(otherwise); }

  // Returns the error this promise failed to or, if it hasn't been resolved
  // yet, the given default value.
  E get_error(E otherwise) { return state_->get_error(otherwise); }

  // Copy constructor that makes sure to ref the state so it doesn't get
  // disposed when 'that' is deleted.
  promise_t(const promise_t<T, E> &that) : state_(that.state_) { state_->ref(); }

  // Adds a callback to be invoked when (if) this promise is successfully
  // resolved. If this promise has already been resolved the action is invoked
  // with the value immediately.
  void on_success(typename promise_state_t<T, E>::SuccessAction action) {
    state_->on_success(action);
  }

  // Adds a callback to be invoked when (if) this promise fails. If this promise has already been resolved the action is invoked
  // with the value immediately.
  void on_failure(typename promise_state_t<T, E>::FailureAction action) {
    state_->on_failure(action);
  }

  // Returns a new promise that resolves when this one does in the same way,
  // but on success the value will be the result of applying the mapping to the
  // value of this promise rather than the value itself. Failures are just
  // passed through directly.
  template <typename T2>
  promise_t<T2, E> then(callback_t<T2(T)> mapper);

  // Assignment operator, also needs to ensure that states are reffed and
  // dereffed appropriately.
  promise_t<T, E> &operator=(const promise_t<T, E> &that) {
    that.state_->ref();
    state_->deref();
    state_ = that.state_;
    return *this;
  }

  // Returns a fresh empty promise.
  static promise_t<T, E> empty();

private:
  template <typename T2>
  static void map_and_fulfill(promise_t<T2, E> dest, callback_t<T2(T)> mapper,
      T value);
  template <typename T2>
  static void pass_on_failure(promise_t<T2, E> dest, E error);

  promise_t<T, E>(promise_state_t<T, E> *state) : state_(state) { state->ref(); }
  promise_state_t<T, E> *state_;
};

} // namespace tclib

#endif // _TCLIB_PROMISE_HH
