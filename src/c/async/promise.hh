//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_PROMISE_HH
#define _TCLIB_PROMISE_HH

#include "c/stdc.h"
#include "c/stdvector.hh"
#include "sync/intex.hh"
#include "sync/mutex.hh"
#include "utils/callback.hh"
#include "utils/refcount.hh"

BEGIN_C_INCLUDES
#include "async/promise.h"
END_C_INCLUDES

namespace tclib {

template <typename T, typename E = void*>
class promise_state_t : public refcount_shared_t {
public:
  typedef callback_t<void(T)> ValueCallback;
  typedef callback_t<void(E)> ErrorCallback;
  promise_state_t();
  virtual ~promise_state_t();
  virtual fat_bool_t fulfill(const T &value);
  virtual fat_bool_t reject(const E &value);
  bool is_settled();
  bool is_fulfilled();
  bool is_rejected();
  const T &peek_value(const T &if_unfulfilled);
  const E &peek_error(const E &if_unfulfilled);
  void on_fulfill(ValueCallback action);
  void on_reject(ErrorCallback action);

protected:
  typedef enum {
    psPending = 0,
    psSettling = 1,
    psFulfilled = 2,
    psRejected = 3
  } state_t;

  atomic_int32_t state_;
  // These will return the raw value or error; only call them after the
  // promise has been fulfilled.
  const T &unsafe_get_value();
  const E &unsafe_get_error();
  NativeMutex guard_;
  std::vector<ValueCallback> on_fulfills_;
  std::vector<ErrorCallback> on_failures_;

protected:
  virtual size_t instance_size() { return sizeof(*this); }

private:
  // These must be used to set the value or error. If you try to set them by
  // assigning to one of the get_* methods you're going to have a bad time.
  void unsafe_set_value(const T &value);
  void unsafe_set_error(const E &error);
  // The way values and errors are stored is a little intricate. Because of
  // strict aliasing you can't just cast your way from the blocks of memory to
  // the values, but -- the process of initializing the memory produces a
  // pointer which is stored and used when reading the value.
  union {
    uint8_t as_value[sizeof(T)];
    uint8_t as_error[sizeof(E)];
  } memory_;
  // Important note: these are only valid after the promise has been fulfilled.
  union {
    T *as_value;
    E *as_error;
  } pointers_;
};

// The result of an operation that may or may not have completed.
//
// There is always a heap allocated component to a promise but to avoid having
// to explicitly keep track of ownership of that component the heap state is
// allocated implicitly and refcounted so the code that passes around promises
// don't have to worry about ownership, which would be unmanageable.
//
// A plain promise is not thread safe; use a sync promise if you need to share
// promises across threads.
//
// The terminology used around promises is as follows. A promise starts out
// unresolved and, once its value has been set, is said to have become resolved.
// Resolving it successfully is called fulfilling, and the outcome of the
// successful fulfillment is called the promise's value. Resolving it
// unsuccessfully is called failing, and the outcome of the failure is called
// an error.
template <typename T, typename E = void*>
class promise_t : public refcount_reference_t< promise_state_t<T, E> > {
public:
  // This is so long we need a shorthand.
  typedef refcount_reference_t< promise_state_t<T, E> > super_t;

  // Fulfills this promise, causing the value to be set and any actions to be
  // performed, but only if this promise is currently empty. Returns true iff
  // that was the case.
  bool fulfill(const T &value) {
    return state()->fulfill(value);
  }

  // Reject this promise, causing the error to be set and any reject actions to
  // be performed, but only if this promise is currently pending. Returns true
  // iff that was the case.
  bool reject(const E &error) {
    return state()->reject(error);
  }

  // Returns true iff this promise has been settled, that is, fulfilled or
  // rejected.
  bool is_settled() { return state()->is_settled(); }

  // Returns true iff this promise has been fulfilled successfully.
  bool is_fulfilled() { return state()->is_fulfilled(); }

  // Returns true iff this promise has been rejected successfully.
  bool is_rejected() { return state()->is_rejected(); }

  // Returns the value this promise resolved to or, if it hasn't been resolved
  // yet, the given default value.
  const T &peek_value(const T &otherwise) { return state()->peek_value(otherwise); }

  // Returns the error this promise failed to or, if it hasn't been resolved
  // yet, the given default value.
  const E &peek_error(const E &otherwise) { return state()->peek_error(otherwise); }

  // Adds a callback to be invoked when (if) this promise is successfully
  // resolved. If this promise has already been resolved the action is invoked
  // with the value immediately.
  void on_fulfill(typename promise_state_t<T, E>::ValueCallback action) {
    state()->on_fulfill(action);
  }

  // Adds a callback to be invoked when (if) this promise fails. If this promise has already been resolved the action is invoked
  // with the value immediately.
  void on_reject(typename promise_state_t<T, E>::ErrorCallback action) {
    state()->on_reject(action);
  }

  // Returns a new promise that resolves when this one does in the same way,
  // but on success the value will be the result of applying the mapping to the
  // value of this promise rather than the value itself. Failures are just
  // passed through directly.
  template <typename T2>
  promise_t<T2, E> then(callback_t<T2(T)> mapper);

  template <typename T2, typename E2>
  promise_t<T2, E2> then(callback_t<T2(T)> vmap, callback_t<E2(E)> emap);

  // Returns a fresh pending promise.
  static promise_t<T, E> pending();

protected:
  promise_t<T, E>(promise_state_t<T, E> *state) : super_t(state) { }

  template <typename T2, typename E2>
  static void map_and_fulfill(promise_t<T2, E2> dest, callback_t<T2(T)> mapper,
      T value);
  template <typename T2, typename E2>
  static void map_and_reject(promise_t<T2, E2> dest, callback_t<E2(E)> mapper,
      E error);
  template <typename T2>
  static void pass_on_rejection(promise_t<T2, E> dest, E error);

  promise_state_t<T, E> *state() { return super_t::refcount_shared(); }
};

// Returns a new opaque promise that behaves the same as the given C++ promise.
opaque_promise_t *to_opaque_promise(promise_t<opaque_t, opaque_t> value);

class NativeSemaphore;

template <typename T, typename E = void*>
class sync_promise_state_t : public promise_state_t<T, E> {
public:
  sync_promise_state_t();
  ~sync_promise_state_t() { }
  fat_bool_t wait(Duration timeout);
  virtual fat_bool_t fulfill(const T &value);
  virtual fat_bool_t reject(const E &value);
protected:
  virtual size_t instance_size() { return sizeof(*this); }

private:

  // Drawbridge that gets lowered when the promise gets its value.
  Drawbridge drawbridge_;
};

// A sync promise is like a promise but safe to share between threads. Also, you
// can wait for a sync promise to settle.
//
// Converting a sync promise to a plain one is fine, it will retain its thread
// safety properties.
template <typename T, typename E = void*>
class sync_promise_t : public promise_t<T, E> {
public:
  // Returns a fresh empty promise.
  static sync_promise_t<T, E> pending();

  // Blocks this thread until this promise has been fulfilled. Once this returns
  // you can use the peek_ methods to get the value/error.
  fat_bool_t wait(Duration timeout = Duration::unlimited()) { return state()->wait(timeout); }

private:
  sync_promise_state_t<T, E> *state() { return static_cast<sync_promise_state_t<T, E>*>(promise_t<T, E>::state()); }

  sync_promise_t<T, E>(sync_promise_state_t<T, E> *state) : promise_t<T, E>(state) { }
};

} // namespace tclib

#endif // _TCLIB_PROMISE_HH
