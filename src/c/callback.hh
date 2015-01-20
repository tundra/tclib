//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_CALLBACK_HH
#define _TCLIB_CALLBACK_HH

// Callback infrastructure.
//
// Strictly, this file should be split in two and all the implementations should
// be in an -inl file but there is so much redundancy with all the different
// arg counts that introducing a new source is more of a problem than having
// everything in the same file.

#include "stdc.h"
#include "refcount.hh"

namespace tclib {

// An opaque wrapper around a function or method pointer. Ideally we'd just cast
// them both to void*s but the language doesn't allow that, nor does it really
// make sense, so instead this wrapper does the appropriate alternative
// gymnastics to be able to view them under the same type.
class opaque_invoker_t {
public:
  template <typename T>
  opaque_invoker_t(T invoker) {
    // Bitcast the invoker into the pointer. Function and method pointer
    // representations are funky and this is the only way I've found that
    // actually works across platforms without making the compilers nervous.
    memcpy(data_, &invoker, sizeof(T));
  }

  opaque_invoker_t() {
    memset(data_, 0, sizeof(data_));
  }

  // Was this invoker initialized using the no-arg constructor, or with a NULL
  // function pointer.
  bool is_empty() {
    return data_[0] == NULL;
  }

  // Cast the opaque pointer back to the original type. The caller is
  // responsible for ensuring that this cast makes sense.
  template <typename F>
  inline F open() {
    F result = NULL;
    memcpy(&result, data_, sizeof(F));
    return result;
  }

  // The max size a function or method pointer can have in bytes.
  static const size_t kMaxSize = 2 * sizeof(void*);

private:
  // The raw data that holds the invoker. This is stored in an array of void*s
  // because the type doesn't really matter but for is_empty it's convenient
  // to be able to read a decent-size chunk of the data as one word so to not
  // have to do too much casting which makes strict aliasing uncomfortable we
  // keep the data under void** the whole time.
  void *data_[kMaxSize / sizeof(void*)];
};

// Implementation shared between binders. Also, this is the stuff that can be
// used even if you don't know what kind of binder you're dealing with. Binders
// are ref counted and shared between callbacks so the main purpose of this code
// is to keep track of that.
class abstract_binder_t : public refcount_shared_t {
public:
  class no_arg_t {
  private:
    no_arg_t();
  };

  // How was this binder allocated? Is it shared so that it mustn't be freed or
  // was it allocated on the heap?
  typedef enum {
    amShared,
    amAlloced
  } alloc_mode_t;

  abstract_binder_t(alloc_mode_t mode)
    : mode_(mode) { }

  virtual void dispose() {
    if (mode_ == amAlloced)
      // To make refcounting as cheap as possible we do it on shared instances
      // as well and then only distinguish the types when the count reaches
      // 0, which it may do any number of times for shared binders.
      delete this;
  }

private:
  alloc_mode_t mode_;
};

// Abstract type that implements binding of function parameters. These are not
// type safe on their own, they need to be used together with callbacks. The
// type parameters are the expected argument parameters. The parameters of the
// subtypes are the bound parameters (starting with B) followed by the remaining
// arguments (starting with A). This can be somewhat confusing so watch out.
template <typename R,
          typename A0 = abstract_binder_t::no_arg_t,
          typename A1 = abstract_binder_t::no_arg_t,
          typename A2 = abstract_binder_t::no_arg_t>
class binder_t : public abstract_binder_t {
public:
  binder_t(alloc_mode_t mode) : abstract_binder_t(mode) { }

  // Methods for invoking the bound function with the bound parameters and
  // also the given arguments. Only one of these is value and the callback
  // knows which one.
  virtual R call(opaque_invoker_t invoker) = 0;
  virtual R call(opaque_invoker_t invoker, A0 a0) = 0;
  virtual R call(opaque_invoker_t invoker, A0 a0, A1 a1) = 0;
  virtual R call(opaque_invoker_t invoker, A0 a0, A1 a1, A2 a2) = 0;
};

// A binder that binds no parameters. As with all the binders, this one is
// spectacularly type unsafe if not used carefully.
template <typename R,
          typename A0 = abstract_binder_t::no_arg_t,
          typename A1 = abstract_binder_t::no_arg_t,
          typename A2 = abstract_binder_t::no_arg_t>
class function_binder_0_t : public binder_t<R, A0, A1, A2> {
public:
  function_binder_0_t(abstract_binder_t::alloc_mode_t mode)
    : binder_t<R, A0, A1, A2>(mode) { }

  virtual R call(opaque_invoker_t invoker) {
    return (invoker.open<R(*)()>())();
  }

  virtual R call(opaque_invoker_t invoker, A0 a0) {
    return (invoker.open<R(*)(A0)>())(a0);
  }

  virtual R call(opaque_invoker_t invoker, A0 a0, A1 a1) {
    return (invoker.open<R(*)(A0, A1)>())(a0, a1);
  }

  virtual R call(opaque_invoker_t invoker, A0 a0, A1 a1, A2 a2) {
    return (invoker.open<R(*)(A0, A1, A2)>())(a0, a1, a2);
  }

  // For each choice of template parameters, returns the same shared binder
  // instance.
  static function_binder_0_t<R, A0, A1, A2> *shared_instance() {
    // This is not strictly thread safe but the worst that can happen if there
    // is a race condition during initialization should be leaking shared
    // instances which is unfortunate but should be benign.
    static function_binder_0_t<R, A0, A1, A2> *instance = NULL;
    if (instance == NULL)
      instance = new function_binder_0_t<R, A0, A1, A2>(abstract_binder_t::amShared);
    return instance;
  }
};

template <typename R,
          typename A0 = abstract_binder_t::no_arg_t,
          typename A1 = abstract_binder_t::no_arg_t,
          typename A2 = abstract_binder_t::no_arg_t>
class method_binder_0_t : public binder_t<R, A0*, A1, A2> {
public:
  method_binder_0_t(abstract_binder_t::alloc_mode_t mode)
    : binder_t<R, A0*, A1, A2>(mode) { }

  virtual R call(opaque_invoker_t invoker) {
    return R();
  }

  virtual R call(opaque_invoker_t invoker, A0 *a0) {
    // "And the winner of gnarliest C++ expression of the year goes to..."
    return (a0->*(invoker.open<R(A0::*)()>()))();
  }

  virtual R call(opaque_invoker_t invoker, A0 *a0, A1 a1) {
    return (a0->*(invoker.open<R(A0::*)(A1)>()))(a1);
  }

  virtual R call(opaque_invoker_t invoker, A0 *a0, A1 a1, A2 a2) {
    return (a0->*(invoker.open<R(A0::*)(A1, A2)>()))(a1, a2);
  }

  // For each choice of template parameters, returns the same shared binder
  // instance.
  static method_binder_0_t<R, A0, A1, A2> *shared_instance() {
    // See function_binder_0_t::shared_instance for the issues around
    // concurrency.
    static method_binder_0_t<R, A0, A1, A2> *instance = NULL;
    if (instance == NULL)
      instance = new method_binder_0_t<R, A0, A1, A2>(abstract_binder_t::amShared);
    return instance;
  }

};

// A binder that binds a single parameter. As with all the binders, this one is
// spectacularly type unsafe if not used carefully.
template <typename R,
          typename B0 = abstract_binder_t::no_arg_t,
          typename A1 = abstract_binder_t::no_arg_t,
          typename A2 = abstract_binder_t::no_arg_t,
          typename A3 = abstract_binder_t::no_arg_t>
class function_binder_1_t : public binder_t<R, A1, A2, A3> {
public:
  function_binder_1_t(B0 b0)
    : binder_t<R, A1, A2, A3>(abstract_binder_t::amAlloced)
    , b0_(b0) { }

  virtual R call(opaque_invoker_t invoker) {
    return (invoker.open<R(*)(B0)>())(b0_);
  }

  virtual R call(opaque_invoker_t invoker, A1 a1) {
    return (invoker.open<R(*)(B0, A1)>())(b0_, a1);
  }

  virtual R call(opaque_invoker_t invoker, A1 a1, A2 a2) {
    return (invoker.open<R(*)(B0, A1, A2)>())(b0_, a1, a2);
  }

  virtual R call(opaque_invoker_t invoker, A1 a1, A2 a2, A3 a3) {
    return (invoker.open<R(*)(B0, A1, A2, A3)>())(b0_, a1, a2, a3);
  }

private:
  B0 b0_;
};

template <typename R,
          typename B0 = abstract_binder_t::no_arg_t,
          typename A1 = abstract_binder_t::no_arg_t,
          typename A2 = abstract_binder_t::no_arg_t,
          typename A3 = abstract_binder_t::no_arg_t>
class method_binder_1_t : public binder_t<R, A1, A2, A3> {
public:
  method_binder_1_t(B0 *b0)
    : binder_t<R, A1, A2, A3>(abstract_binder_t::amAlloced)
    , b0_(b0) { }

  virtual R call(opaque_invoker_t invoker) {
    return (b0_->*(invoker.open<R(B0::*)()>()))();
  }

  virtual R call(opaque_invoker_t invoker, A1 a1) {
    return (b0_->*(invoker.open<R(B0::*)(A1)>()))(a1);
  }

  virtual R call(opaque_invoker_t invoker, A1 a1, A2 a2) {
    return (b0_->*(invoker.open<R(B0::*)(A1, A2)>()))(a1, a2);
  }

  virtual R call(opaque_invoker_t invoker, A1 a1, A2 a2, A3 a3) {
    return (b0_->*(invoker.open<R(B0::*)(A1, A2, A3)>()))(a1, a2, a3);
  }

private:
  B0 *b0_;
};

// A binder that binds two parameters. As with all the binders, this one is
// spectacularly type unsafe if not used carefully.
template <typename R,
          typename B0 = abstract_binder_t::no_arg_t,
          typename B1 = abstract_binder_t::no_arg_t,
          typename A2 = abstract_binder_t::no_arg_t,
          typename A3 = abstract_binder_t::no_arg_t,
          typename A4 = abstract_binder_t::no_arg_t>
class function_binder_2_t : public binder_t<R, A2, A3, A4> {
public:
  function_binder_2_t(B0 b0, B1 b1)
    : binder_t<R, A2, A3, A4>(abstract_binder_t::amAlloced)
    , b0_(b0)
    , b1_(b1) { }

  virtual R call(opaque_invoker_t invoker) {
    return (invoker.open<R(*)(B0, B1)>())(b0_, b1_);
  }

  virtual R call(opaque_invoker_t invoker, A2 a2) {
    return (invoker.open<R(*)(B0, B1, A2)>())(b0_, b1_, a2);
  }

  virtual R call(opaque_invoker_t invoker, A2 a2, A3 a3) {
    return (invoker.open<R(*)(B0, B1, A2, A3)>())(b0_, b1_, a2, a3);
  }

  virtual R call(opaque_invoker_t invoker, A2 a2, A3 a3, A4 a4) {
    return (invoker.open<R(*)(B0, B1, A2, A3, A4)>())(b0_, b1_, a2, a3, a4);
  }

private:
  B0 b0_;
  B1 b1_;
};

// A binder that binds three parameters. As with all the binders, this one is
// spectacularly type unsafe if not used carefully.
template <typename R,
          typename B0 = abstract_binder_t::no_arg_t,
          typename B1 = abstract_binder_t::no_arg_t,
          typename B2 = abstract_binder_t::no_arg_t,
          typename A3 = abstract_binder_t::no_arg_t,
          typename A4 = abstract_binder_t::no_arg_t,
          typename A5 = abstract_binder_t::no_arg_t>
class function_binder_3_t : public binder_t<R, A3, A4, A5> {
public:
  function_binder_3_t(B0 b0, B1 b1, B2 b2)
    : binder_t<R, A3, A4, A5>(abstract_binder_t::amAlloced)
    , b0_(b0)
    , b1_(b1)
    , b2_(b2) { }

  virtual R call(opaque_invoker_t invoker) {
    return (invoker.open<R(*)(B0, B1, B2)>())(b0_, b1_, b2_);
  }

  virtual R call(opaque_invoker_t invoker, A3 a3) {
    return (invoker.open<R(*)(B0, B1, B2, A3)>())(b0_, b1_, b2_, a3);
  }

  virtual R call(opaque_invoker_t invoker, A3 a3, A4 a4) {
    return (invoker.open<R(*)(B0, B1, B2, A3, A4)>())(b0_, b1_, b2_, a3, a4);
  }

  virtual R call(opaque_invoker_t invoker, A3 a3, A4 a4, A5 a5) {
    return (invoker.open<R(*)(B0, B1, B2, A3, A4, A5)>())(b0_, b1_, b2_, a3, a4, a5);
  }

private:
  B0 b0_;
  B1 b1_;
  B2 b2_;
};

template <typename R,
          typename B0 = abstract_binder_t::no_arg_t,
          typename B1 = abstract_binder_t::no_arg_t,
          typename A2 = abstract_binder_t::no_arg_t,
          typename A3 = abstract_binder_t::no_arg_t,
          typename A4 = abstract_binder_t::no_arg_t>
class method_binder_2_t : public binder_t<R, A2, A3, A4> {
public:
  method_binder_2_t(B0 *b0, B1 b1)
    : binder_t<R, A2, A3, A4>(abstract_binder_t::amAlloced)
    , b0_(b0)
    , b1_(b1) { }

  virtual R call(opaque_invoker_t invoker) {
    return (b0_->*(invoker.open<R(B0::*)(B1)>()))(b1_);
  }

  virtual R call(opaque_invoker_t invoker, A2 a2) {
    return (b0_->*(invoker.open<R(B0::*)(B1, A2)>()))(b1_, a2);
  }

  virtual R call(opaque_invoker_t invoker, A2 a2, A3 a3) {
    return (b0_->*(invoker.open<R(B0::*)(B1, A2, A3)>()))(b1_, a2, a3);
  }

  virtual R call(opaque_invoker_t invoker, A2 a2, A3 a3, A4 a4) {
    return (b0_->*(invoker.open<R(B0::*)(B1, A2, A3, A4)>()))(b1_, a2, a3, a4);
  }

private:
  B0 *b0_;
  B1 b1_;
};

template <typename R,
          typename B0 = abstract_binder_t::no_arg_t,
          typename B1 = abstract_binder_t::no_arg_t,
          typename B2 = abstract_binder_t::no_arg_t,
          typename A3 = abstract_binder_t::no_arg_t,
          typename A4 = abstract_binder_t::no_arg_t,
          typename A5 = abstract_binder_t::no_arg_t>
class method_binder_3_t : public binder_t<R, A3, A4, A5> {
public:
  method_binder_3_t(B0 *b0, B1 b1, B2 b2)
    : binder_t<R, A3, A4, A5>(abstract_binder_t::amAlloced)
    , b0_(b0)
    , b1_(b1)
    , b2_(b2) { }

  virtual R call(opaque_invoker_t invoker) {
    return (b0_->*(invoker.open<R(B0::*)(B1, B2)>()))(b1_, b2_);
  }

  virtual R call(opaque_invoker_t invoker, A3 a3) {
    return (b0_->*(invoker.open<R(B0::*)(B1, B2, A3)>()))(b1_, b2_, a3);
  }

  virtual R call(opaque_invoker_t invoker, A3 a3, A4 a4) {
    return (b0_->*(invoker.open<R(B0::*)(B1, B2, A3, A4)>()))(b1_, b2_, a3, a4);
  }

  virtual R call(opaque_invoker_t invoker, A3 a3, A4 a4, A5 a5) {
    return (b0_->*(invoker.open<R(B0::*)(B1, B2, A3, A4, A5)>()))(b1_, b2_, a3, a4, a5);
  }

private:
  B0 *b0_;
  B1 b1_;
  B2 b2_;
};

// Functionality shared between callbacks. The main purpose of this is to
// keep track of the types involved and allow the same binder to be passed
// around and disposed as appropriate, without the client having to keep track
// of it explicitly.
class abstract_callback_t : public refcount_reference_t<abstract_binder_t> {
public:
  // Initializes an empty callback.
  abstract_callback_t() : refcount_reference_t() { }

  // Copy constructor that makes sure to ref the binder so it doesn't get
  // disposed when 'that' is deleted.
  abstract_callback_t(const abstract_callback_t &that)
    : refcount_reference_t(that)
    , invoker_(that.invoker_) { }

  // Assignment operator, also needs to ensure that binders are reffed and
  // dereffed appropriately.
  abstract_callback_t &operator=(const abstract_callback_t &that) {
    refcount_reference_t::operator=(that);
    invoker_ = that.invoker_;
    return *this;
  }

  // Has this callback been set to an actual value?
  bool is_empty() {
    return invoker_.is_empty();
  }

  // Alternative name for the shared state. This is visible for testing, there's
  // no need to ever use this directly.
  abstract_binder_t *binder() { return refcount_shared(); }

protected:
  // Binders are born zero-reffed so this way the number of refs and derefs
  // always matches: ref on construction, deref on disposal.
  abstract_callback_t(opaque_invoker_t invoker, abstract_binder_t *binder)
    : refcount_reference_t(binder)
    , invoker_(invoker) { }

  // The binder to call to invoke this callback.
  opaque_invoker_t invoker_;
};

// A generic callback. The actual implementation is in the specializations.
template <typename S>
class callback_t;

// Marker type that indicates a null/empty callback.
struct null_callback_t { };

// Returns a value that can be passed as any callback. This relies on an
// implicit conversion happening so the types may not always add up if you're
// doing complex type stuff.
static inline struct null_callback_t empty_callback() {
  null_callback_t result;
  return result;
}



// A nullary no-state callback.
template <typename R>
class callback_t<R(void)> : public abstract_callback_t {
public:
  // This is mainly used for consistency and validation: the binder used by this
  // callback should be of this type.
  typedef binder_t<R> my_binder_t;

  callback_t() : abstract_callback_t() { }
  callback_t(opaque_invoker_t invoker, my_binder_t *binder) : abstract_callback_t(invoker, binder) { }
  callback_t(null_callback_t) : abstract_callback_t() { }
  callback_t(R (*invoker)(void))
    : abstract_callback_t(invoker, function_binder_0_t<R>::shared_instance()) { }

  R operator()() {
    return (static_cast<my_binder_t*>(binder()))->call(invoker_);
  }
};

template <typename R>
callback_t<R(void)> new_callback(R (*invoker)(void)) {
  return invoker;
}

template <typename R, typename B0>
callback_t<R(void)> new_callback(R (*invoker)(B0), B0 b0) {
  return callback_t<R(void)>(invoker, new function_binder_1_t<R, B0>(b0));
}

template <typename R, typename B0>
callback_t<R(void)> new_callback(R (B0::*invoker)(void), B0 *b0) {
  return callback_t<R(void)>(invoker, new method_binder_1_t<R, B0>(b0));
}

template <typename R, typename B0, typename B1>
callback_t<R(void)> new_callback(R (*invoker)(B0, B1), B0 b0, B1 b1) {
  return callback_t<R(void)>(invoker, new function_binder_2_t<R, B0, B1>(b0, b1));
}

template <typename R, typename B0, typename B1>
callback_t<R(void)> new_callback(R (B0::*invoker)(B1), B0 *b0, B1 b1) {
  return callback_t<R(void)>(invoker, new method_binder_2_t<R, B0, B1>(b0, b1));
}

template <typename R, typename B0, typename B1, typename B2>
callback_t<R(void)> new_callback(R (*invoker)(B0, B1, B2), B0 b0, B1 b1, B2 b2) {
  return callback_t<R(void)>(invoker, new function_binder_3_t<R, B0, B1, B2>(b0, b1, b2));
}

template <typename R, typename B0, typename B1, typename B2>
callback_t<R(void)> new_callback(R (B0::*invoker)(B1, B2), B0 *b0, B1 b1, B2 b2) {
  return callback_t<R(void)>(invoker, new method_binder_3_t<R, B0, B1, B2>(b0, b1, b2));
}

template <typename T>
static void destructor_trampoline(T *that) {
  that->~T();
}

// Returns a callback that, given an instance of type T, invokes that instance's
// destructor. Note that this doesn't delete the instance, it just calls the
// destructor.
template <typename T>
callback_t<void(T*)> new_destructor_callback() {
  return callback_t<void(T*)>(destructor_trampoline<T>, function_binder_0_t<void, T*>::shared_instance());
}

// Returns a callback that, when invoked, invokes the given instance's
// destructor. Note that this doesn't delete the instance, it just calls the
// destructor.
template <typename T>
callback_t<void(void)> new_destructor_callback(T *t) {
  return callback_t<void(void)>(destructor_trampoline<T>, new function_binder_1_t<void, T*>(t));
}

template <typename R, typename A0>
class callback_t<R(A0)> : public abstract_callback_t {
public:
  // This is mainly used for consistency and validation: the binder used by this
  // callback should be of this type.
  typedef binder_t<R, A0> my_binder_t;

  callback_t() : abstract_callback_t() { }
  callback_t(opaque_invoker_t invoker, my_binder_t *binder)
    : abstract_callback_t(invoker, binder) { }
  callback_t(null_callback_t) : abstract_callback_t() { }
  callback_t(R (*invoker)(A0))
    : abstract_callback_t(invoker, function_binder_0_t<R, A0>::shared_instance()) { }

  R operator()(A0 a0) {
    return (static_cast<my_binder_t*>(binder()))->call(invoker_, a0);
  }
};

template <typename R, typename A0>
callback_t<R(A0)> new_callback(R (*invoker)(A0)) {
  return invoker;
}

template <typename R, typename A0, typename B0>
callback_t<R(A0)> new_callback(R (*invoker)(B0, A0), B0 b0) {
  return callback_t<R(A0)>(invoker, new function_binder_1_t<R, B0, A0>(b0));
}

template <typename R, typename A0, typename B0, typename B1>
callback_t<R(A0)> new_callback(R (*invoker)(B0, B1, A0), B0 b0, B1 b1) {
  return callback_t<R(A0)>(invoker, new function_binder_2_t<R, B0, B1, A0>(b0, b1));
}

template <typename R, typename A0, typename B0, typename B1, typename B2>
callback_t<R(A0)> new_callback(R (*invoker)(B0, B1, B2, A0), B0 b0, B1 b1, B2 b2) {
  return callback_t<R(A0)>(invoker, new function_binder_3_t<R, B0, B1, B2, A0>(b0, b1, b2));
}

template <typename R, typename A0>
callback_t<R(A0*)> new_callback(R (A0::*invoker)(void)) {
  return callback_t<R(A0*)>(invoker, method_binder_0_t<R, A0>::shared_instance());
}

template <typename R, typename A0, typename B0>
callback_t<R(A0)> new_callback(R (B0::*invoker)(A0), B0 *b0) {
  return callback_t<R(A0)>(invoker, new method_binder_1_t<R, B0, A0>(b0));
}

template <typename R, typename A0, typename B0, typename B1>
callback_t<R(A0)> new_callback(R (B0::*invoker)(B1, A0), B0 *b0, B1 b1) {
  return callback_t<R(A0)>(invoker, new method_binder_2_t<R, B0, B1, A0>(b0, b1));
}

template <typename R, typename A0, typename B0, typename B1, typename B2>
callback_t<R(A0)> new_callback(R (B0::*invoker)(B1, B2, A0), B0 *b0, B1 b1, B2 b2) {
  return callback_t<R(A0)>(invoker, new method_binder_3_t<R, B0, B1, B2, A0>(b0, b1, b2));
}

template <typename R, typename A0, typename A1>
class callback_t<R(A0, A1)> : public abstract_callback_t {
public:
  // This is mainly used for consistency and validation: the binder used by this
  // callback should be of this type.
  typedef binder_t<R, A0, A1> my_binder_t;

  callback_t() : abstract_callback_t() { }
  callback_t(opaque_invoker_t invoker, my_binder_t *binder) : abstract_callback_t(invoker, binder) { }
  callback_t(null_callback_t) : abstract_callback_t() { }
  callback_t(R (*invoker)(A0, A1))
    : abstract_callback_t(invoker, function_binder_0_t<R, A0, A1>::shared_instance()) { }

  R operator()(A0 a0, A1 a1) {
    return (static_cast<my_binder_t*>(binder()))->call(invoker_, a0, a1);
  }
};

template <typename R, typename A0, typename A1>
callback_t<R(A0, A1)> new_callback(R (*invoker)(A0, A1)) {
  return invoker;
}

template <typename R, typename A0, typename A1, typename B0>
callback_t<R(A0, A1)> new_callback(R (*invoker)(B0, A0, A1), B0 b0) {
  return callback_t<R(A0, A1)>(invoker, new function_binder_1_t<R, B0, A0, A1>(b0));
}

template <typename R, typename A0, typename A1, typename B0, typename B1>
callback_t<R(A0, A1)> new_callback(R (*invoker)(B0, B1, A0, A1), B0 b0, B1 b1) {
  return callback_t<R(A0, A1)>(invoker, new function_binder_2_t<R, B0, B1, A0, A1>(b0, b1));
}

template <typename R, typename A0, typename A1, typename B0, typename B1, typename B2>
callback_t<R(A0, A1)> new_callback(R (*invoker)(B0, B1, B2, A0, A1), B0 b0, B1 b1, B2 b2) {
  return callback_t<R(A0, A1)>(invoker, new function_binder_3_t<R, B0, B1, B2, A0, A1>(b0, b1, b2));
}

template <typename R, typename A0, typename A1>
callback_t<R(A0*, A1)> new_callback(R (A0::*invoker)(A1)) {
  return callback_t<R(A0*, A1)>(invoker, method_binder_0_t<R, A0, A1>::shared_instance());
}

template <typename R, typename A0, typename A1, typename B0>
callback_t<R(A0, A1)> new_callback(R (B0::*invoker)(A0, A1), B0* b0) {
  return callback_t<R(A0, A1)>(invoker, new method_binder_1_t<R, B0, A0, A1>(b0));
}

template <typename R, typename A0, typename A1, typename B0, typename B1>
callback_t<R(A0, A1)> new_callback(R (B0::*invoker)(B1, A0, A1), B0* b0, B1 b1) {
  return callback_t<R(A0, A1)>(invoker, new method_binder_2_t<R, B0, B1, A0, A1>(b0, b1));
}

template <typename R, typename A0, typename A1, typename B0, typename B1, typename B2>
callback_t<R(A0, A1)> new_callback(R (B0::*invoker)(B1, B2, A0, A1), B0* b0, B1 b1, B2 b2) {
  return callback_t<R(A0, A1)>(invoker, new method_binder_3_t<R, B0, B1, B2, A0, A1>(b0, b1, b2));
}

template <typename R, typename A0, typename A1, typename A2>
class callback_t<R(A0, A1, A2)> : public abstract_callback_t {
public:
  // This is mainly used for consistency and validation: the binder used by this
  // callback should be of this type.
  typedef binder_t<R, A0, A1, A2> my_binder_t;

  callback_t() : abstract_callback_t() { }
  callback_t(opaque_invoker_t invoker, my_binder_t *binder) : abstract_callback_t(invoker, binder) { }
  callback_t(null_callback_t) : abstract_callback_t() { }
  callback_t(R (*invoker)(A0, A1, A2))
    : abstract_callback_t(invoker, function_binder_0_t<R, A0, A1, A2>::shared_instance()) { }

  R operator()(A0 a0, A1 a1, A2 a2) {
    return (static_cast<my_binder_t*>(binder()))->call(invoker_, a0, a1, a2);
  }
};

template <typename R, typename A0, typename A1, typename A2>
callback_t<R(A0, A1, A2)> new_callback(R (*invoker)(A0, A1, A2)) {
  return invoker;
}

template <typename R, typename A0, typename A1, typename A2, typename B0>
callback_t<R(A0, A1, A2)> new_callback(R (*invoker)(B0, A0, A1, A2), B0 b0) {
  return callback_t<R(A0, A1, A2)>(invoker, new function_binder_1_t<R, B0, A0, A1, A2>(b0));
}

template <typename R, typename A0, typename A1, typename A2, typename B0, typename B1>
callback_t<R(A0, A1, A2)> new_callback(R (*invoker)(B0, B1, A0, A1, A2), B0 b0, B1 b1) {
  return callback_t<R(A0, A1, A2)>(invoker, new function_binder_2_t<R, B0, B1, A0, A1, A2>(b0, b1));
}

template <typename R, typename A0, typename A1, typename A2, typename B0, typename B1, typename B2>
callback_t<R(A0, A1, A2)> new_callback(R (*invoker)(B0, B1, B2, A0, A1, A2), B0 b0, B1 b1, B2 b2) {
  return callback_t<R(A0, A1, A2)>(invoker, new function_binder_3_t<R, B0, B1, B2, A0, A1, A2>(b0, b1, b2));
}

template <typename R, typename A0, typename A1, typename A2>
callback_t<R(A0*, A1, A2)> new_callback(R (A0::*invoker)(A1, A2)) {
  return callback_t<R(A0*, A1, A2)>(invoker, method_binder_0_t<R, A0, A1, A2>::shared_instance());
}

template <typename R, typename A0, typename A1, typename A2, typename B0>
callback_t<R(A0, A1, A2)> new_callback(R (B0::*invoker)(A0, A1, A2), B0 *b0) {
  return callback_t<R(A0, A1, A2)>(invoker, new method_binder_1_t<R, B0, A0, A1, A2>(b0));
}

template <typename R, typename A0, typename A1, typename A2, typename B0, typename B1>
callback_t<R(A0, A1, A2)> new_callback(R (B0::*invoker)(B1, A0, A1, A2), B0 *b0, B1 b1) {
  return callback_t<R(A0, A1, A2)>(invoker, new method_binder_2_t<R, B0, B1, A0, A1, A2>(b0, b1));
}

template <typename R, typename A0, typename A1, typename A2, typename B0, typename B1, typename B2>
callback_t<R(A0, A1, A2)> new_callback(R (B0::*invoker)(B1, B2, A0, A1, A2), B0 *b0, B1 b1, B2 b2) {
  return callback_t<R(A0, A1, A2)>(invoker, new method_binder_3_t<R, B0, B1, B2, A0, A1, A2>(b0, b1, b2));
}

} // namespace tclib

#endif // _TCLIB_CALLBACK_HH
