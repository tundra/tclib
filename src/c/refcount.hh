//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_REFCOUNT_HH
#define _TCLIB_REFCOUNT_HH

// Utilities for refcounted data. The pattern this supports has two roles:
// shared data and references. The shared data is the ref counted part and is
// shared between a number of references. The references are passed by value and
// are responsible for counting and dereffing the shared data. Once the last
// reference goes away the shared data is disposed appropriately.

#include "stdc.h"

namespace tclib {

// The ref counted state shared between references.
class refcount_shared_t {
public:
  refcount_shared_t() : refcount_(0) { }
  virtual ~refcount_shared_t() { }

  // Increment refcount.
  void ref() {
    refcount_++;
  }

  // Decremet refcount, possibly disposing this object.
  void deref() {
    if (--refcount_ == 0)
      dispose();
  }

  // Dispose this value appropriately.
  virtual void dispose() {
    delete this;
  }

  // The raw refcount. This is visible for testing, you typically don't want to
  // ever use this for production code.
  size_t refcount() {
    return refcount_;
  }

private:
  size_t refcount_;
};

// A references to shared data of type T, where T should be derived from
// refcount_shared_t.
template <typename T>
class refcount_reference_t {
public:
  // Initializes an empty reference. The empty reference has a NULL shared state
  // which is fine.
  refcount_reference_t()
    : shared_(NULL) { }

  // Copy constructor that makes sure to ref the shared state so it doesn't get
  // disposed when 'that' is deleted.
  refcount_reference_t(const refcount_reference_t<T> &that)
    : shared_(that.shared_) {
    ref_shared();
  }

  // Assignment operator, also needs to ensure that shared state is reffed and
  // dereffed appropriately.
  refcount_reference_t<T> &operator=(const refcount_reference_t<T> &that) {
    if (shared_ != that.shared_) {
      deref_shared();
      shared_ = that.shared_;
      ref_shared();
    }
    return *this;
  }

  // Deref the binder on disposal.
  ~refcount_reference_t() {
    deref_shared();
  }

protected:
  // Wrap the given shared state.
  refcount_reference_t(T *shared)
    : shared_(shared) {
    ref_shared();
  }

  // Returns the shared reference.
  T *refcount_shared() { return shared_; }

  // Sets the shared reference directly. Note that this will not ref the value,
  // if that's the right thing to do you need to do that manually before or
  // after.
  void set_refcount_shared(T *value) { shared_ = value; }

private:
  void ref_shared() {
    if (shared_)
      shared_->ref();
  }

  void deref_shared() {
    if (shared_)
      shared_->deref();
  }

  T *shared_;
};

} // namespace tclib

#endif // _TCLIB_REFCOUNT_HH
