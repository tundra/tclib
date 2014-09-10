//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_CALLBACK_HH
#define _TCLIB_CALLBACK_HH

#include "stdc.h"

namespace tclib {

// Implementation shared between binders. Also, this is the stuff that can be
// used even if you don't know what kind of binder you're dealing with. Binders
// are ref counted and shared between callbacks so the main purpose of this code
// is to keep track of that.
class abstract_binder_t {
public:
  class no_arg_t {
  private:
    no_arg_t();
  };

  abstract_binder_t()
    : refcount_(0) { }

  // Subtypes have nontrivial destructors.
  virtual ~abstract_binder_t() { }

  // Increment refcount.
  void ref() {
    refcount_++;
  }

  // Decremet refcount, possibly deleting the binder.
  void deref() {
    if (--refcount_ == 0)
      delete this;
  }

private:
  size_t refcount_;
};

// Abstract type that implements binding of function parameters. These are not
// type safe on their own, they need to be used together with callbacks. The
// type parameters are the expected argument parameters. The parameters of the
// subtypes are the bound parameters (starting with B) followed by the remaining
// arguments (starting with A). This can be somewhat confusing so watch out.
template <typename R,
          typename A0 = abstract_binder_t::no_arg_t,
          typename A1 = abstract_binder_t::no_arg_t,
          typename A2 = abstract_binder_t::no_arg_t,
          typename A3 = abstract_binder_t::no_arg_t>
class binder_t : public abstract_binder_t {
public:
  // The max size in bytes of any method or function pointer. It seems generally
  // that 2 words are sufficient (and also necessary).
  static const size_t kMaxInvokerSize = (WORD_SIZE << 1);

  // On destruction clear the invoker just to ensure that any code that uses
  // this after destruction fails.
  virtual ~binder_t() {
    memset(raw_invoker_, 0, kMaxInvokerSize);
  }

  // Methods for invoking the bound function with the bound parameters and
  // also the given arguments. Only one of these is value and the callback
  // knows which one.
  virtual R call(void) = 0;
  virtual R call(A0 a0) = 0;
  virtual R call(A0 a0, A1 a1) = 0;
protected:
  template <typename T>
  binder_t(T invoker) {
    // Bitcast the invoker into the pointer. Function and method pointer
    // representations are funky and this is the only way I've found that
    // actually works across platforms without making the compilers nervous.
    memcpy(raw_invoker_, &invoker, sizeof(T));
  }

  template <typename T>
  T invoker() {
    // Bitcast the invoker back to the appropriate type.
    T result = NULL;
    memcpy(&result, raw_invoker_, sizeof(T));
    return result;
  }

  uint8_t raw_invoker_[kMaxInvokerSize];
};

// A binder that binds no parameters. As with all the binders, this one is
// spectacularly type unsafe if not used carefully.
template <typename R,
          typename A0 = abstract_binder_t::no_arg_t,
          typename A1 = abstract_binder_t::no_arg_t,
          typename A2 = abstract_binder_t::no_arg_t,
          typename A3 = abstract_binder_t::no_arg_t>
class function_binder_0_t : public binder_t<R, A0, A1, A2, A3> {
public:
  template <typename T>
  function_binder_0_t(T fun)
    : binder_t<R, A0, A1, A2, A3>(fun) { }

  virtual R call(void) {
    typedef R (*invoker_t)(void);
    invoker_t function = this->template invoker<invoker_t>();
    return function();
  }

  virtual R call(A0 a0) {
    typedef R(*invoker_t)(A0);
    invoker_t function = this->template invoker<invoker_t>();
    return function(a0);
  }

  virtual R call(A0 a0, A1 a1) {
    typedef R(*invoker_t)(A0, A1);
    invoker_t function = this->template invoker<invoker_t>();
    return function(a0, a1);
  }
};

template <typename R,
          typename A0 = abstract_binder_t::no_arg_t,
          typename A1 = abstract_binder_t::no_arg_t,
          typename A2 = abstract_binder_t::no_arg_t,
          typename A3 = abstract_binder_t::no_arg_t>
class method_binder_0_t : public binder_t<R, A0*, A1, A2, A3> {
public:
  template <typename T>
  method_binder_0_t(T method)
    : binder_t<R, A0*, A1, A2, A3>(method) { }

  virtual R call(void) {
    // ignore
    return R();
  }

  virtual R call(A0 *a0) {
    typedef R(A0::*invoker_t)(void);
    invoker_t method = this->template invoker<invoker_t>();
    return (a0->*(method))();
  }

  virtual R call(A0 *a0, A1 a1) {
    typedef R(A0::*invoker_t)(A1 a1);
    invoker_t method = this->template invoker<invoker_t>();
    return (a0->*(method))(a1);
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
  template <typename T>
  function_binder_1_t(T fun, B0 b0)
    : binder_t<R, A1, A2, A3>(fun)
    , b0_(b0) { }

  virtual R call(void) {
    typedef R (*invoker_t)(B0);
    invoker_t function = this->template invoker<invoker_t>();
    return function(b0_);
  }

  virtual R call(A1 a1) {
    typedef R(*invoker_t)(B0, A1);
    invoker_t function = this->template invoker<invoker_t>();
    return function(b0_, a1);
  }

  virtual R call(A1 a1, A2 a2) {
    typedef R(*invoker_t)(B0, A1, A2);
    invoker_t function = this->template invoker<invoker_t>();
    return function(b0_, a1, a2);
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
  template <typename T>
  method_binder_1_t(T method, B0 *b0)
    : binder_t<R, A1, A2, A3>(method)
    , b0_(b0) { }

  virtual R call(void) {
    typedef R(B0::*invoker_t)(void);
    invoker_t method = this->template invoker<invoker_t>();
    return (b0_->*(method))();
  }

  virtual R call(A1 a1) {
    typedef R(B0::*invoker_t)(A1);
    invoker_t method = this->template invoker<invoker_t>();
    return (b0_->*(method))(a1);
  }

  virtual R call(A1 a1, A2 a2) {
    typedef R(B0::*invoker_t)(A1, A2);
    invoker_t method = this->template invoker<invoker_t>();
    return (b0_->*(method))(a1, a2);
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
          typename A3 = abstract_binder_t::no_arg_t>
class function_binder_2_t : public binder_t<R, A2, A3> {
public:
  template <typename T>
  function_binder_2_t(T fun, B0 b0, B1 b1)
    : binder_t<R, A2, A3>(fun)
    , b0_(b0)
    , b1_(b1) { }

  virtual R call(void) {
    typedef R (*invoker_t)(B0, B1);
    invoker_t function = this->template invoker<invoker_t>();
    return function(b0_, b1_);
  }

  virtual R call(A2 a2) {
    typedef R(*invoker_t)(B0, B1, A2);
    invoker_t function = this->template invoker<invoker_t>();
    return function(b0_, b1_, a2);
  }

  virtual R call(A2 a2, A3 a3) {
    typedef R(*invoker_t)(B0, B1, A2, A3);
    invoker_t function = this->template invoker<invoker_t>();
    return function(b0_, b1_, a2, a3);
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
          typename A3 = abstract_binder_t::no_arg_t>
class function_binder_3_t : public binder_t<R, A3> {
public:
  template <typename T>
  function_binder_3_t(T fun, B0 b0, B1 b1, B2 b2)
    : binder_t<R, A3>(fun)
    , b0_(b0)
    , b1_(b1)
    , b2_(b2) { }

  virtual R call(void) {
    typedef R (*invoker_t)(B0, B1, B2);
    invoker_t function = this->template invoker<invoker_t>();
    return function(b0_, b1_, b2_);
  }

  virtual R call(A3 a3) {
    typedef R(*invoker_t)(B0, B1, B2, A3);
    invoker_t function = this->template invoker<invoker_t>();
    return function(b0_, b1_, b2_, a3);
  }

  virtual R call(A3 a3, abstract_binder_t::no_arg_t) {
    return R();
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
          typename A3 = abstract_binder_t::no_arg_t>
class method_binder_2_t : public binder_t<R, A2, A3> {
public:
  template <typename T>
  method_binder_2_t(T method, B0 *b0, B1 b1)
    : binder_t<R, A2, A3>(method)
    , b0_(b0)
    , b1_(b1) { }

  virtual R call(void) {
    typedef R(B0::*invoker_t)(B1);
    invoker_t method = this->template invoker<invoker_t>();
    return (b0_->*(method))(b1_);
  }

  virtual R call(A2 a2) {
    typedef R(B0::*invoker_t)(B1, A2);
    invoker_t method = this->template invoker<invoker_t>();
    return (b0_->*(method))(b1_, a2);
  }

  virtual R call(A2 a2, A3 a3) {
    typedef R(B0::*invoker_t)(B1, A2, A3);
    invoker_t method = this->template invoker<invoker_t>();
    return (b0_->*(method))(b1_, a2, a3);
  }

private:
  B0 *b0_;
  B1 b1_;
};

template <typename R,
          typename B0 = abstract_binder_t::no_arg_t,
          typename B1 = abstract_binder_t::no_arg_t,
          typename B2 = abstract_binder_t::no_arg_t,
          typename A3 = abstract_binder_t::no_arg_t>
class method_binder_3_t : public binder_t<R, A3> {
public:
  template <typename T>
  method_binder_3_t(T method, B0 *b0, B1 b1, B2 b2)
    : binder_t<R, A3>(method)
    , b0_(b0)
    , b1_(b1)
    , b2_(b2) { }

  virtual R call(void) {
    typedef R(B0::*invoker_t)(B1, B2);
    invoker_t method = this->template invoker<invoker_t>();
    return (b0_->*(method))(b1_, b2_);
  }

  virtual R call(A3 a3) {
    typedef R(B0::*invoker_t)(B1, B2, A3);
    invoker_t method = this->template invoker<invoker_t>();
    return (b0_->*(method))(b1_, b2_, a3);
  }

  virtual R call(A3 a3, abstract_binder_t::no_arg_t) {
    return R();
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
class abstract_callback_t {
public:
  // Initializes an empty callback.
  abstract_callback_t() : binder_(NULL) { }

  // Copy constructor that makes sure to ref the binder so it doesn't get
  // disposed when 'that' is deleted.
  abstract_callback_t(const abstract_callback_t &that)
    : binder_(that.binder_) {
    if (binder_)
      binder_->ref();
  }

  // Assignment operator, also needs to ensure that binders are reffed and
  // dereffed appropriately.
  abstract_callback_t &operator=(const abstract_callback_t &that) {
    if (binder_ != that.binder_) {
      if (binder_)
        binder_->deref();
      binder_ = that.binder_;
      if (binder_)
        binder_->ref();
    }
    return *this;
  }

  // Deref the binder on disposal.
  ~abstract_callback_t() {
    if (binder_)
      binder_->deref();
  }

  // Has this callback been set to an actual value?
  bool is_empty() {
    return binder_ == NULL;
  }

protected:
  // Binders are born zero-reffed so this way the number of refs and derefs
  // always matches: ref on construction, deref on disposal.
  abstract_callback_t(abstract_binder_t *binder)
    : binder_(binder) {
    if (binder)
      binder->ref();
  }

  // The binder to call to invoke this callback.
  abstract_binder_t *binder_;
};

// A generic callback. The actual implementation is in the specializations.
template <typename S>
class callback_t;

// A nullary no-state callback.
template <typename R>
class callback_t<R(void)> : public abstract_callback_t {
public:
  // This is mainly used for consistency and validation: the binder used by this
  // callback should be of this type.
  typedef binder_t<R> my_binder_t;

  callback_t() : abstract_callback_t() { }

  R operator()() {
    return (static_cast<my_binder_t*>(binder_))->call();
  }

private:
  template <typename FR>
  friend callback_t<FR(void)> new_callback(FR (*)(void));
  template <typename FR, typename FB0>
  friend callback_t<FR(void)> new_callback(FR (*)(FB0), FB0);
  template <typename FR, typename FB0>
  friend callback_t<FR(void)> new_callback(FR (FB0::*)(void), FB0*);
  template <typename FR, typename FB0, typename FB1>
  friend callback_t<FR(void)> new_callback(FR (*)(FB0, FB1), FB0, FB1);
  template <typename FR, typename FB0, typename FB1>
  friend callback_t<FR(void)> new_callback(FR (FB0::*)(FB1), FB0*, FB1);
  template <typename FR, typename FB0, typename FB1, typename FB2>
  friend callback_t<FR(void)> new_callback(FR (*)(FB0, FB1, FB2), FB0, FB1, FB2);
  template <typename FR, typename FB0, typename FB1, typename FB2>
  friend callback_t<FR(void)> new_callback(FR (FB0::*)(FB1), FB0*, FB1, FB2);
  callback_t(my_binder_t *binder) : abstract_callback_t(binder) { }
};

template <typename R>
callback_t<R(void)> new_callback(R (*invoker)(void)) {
  return callback_t<R(void)>(new function_binder_0_t<R>(invoker));
}

template <typename R, typename B0>
callback_t<R(void)> new_callback(R (*invoker)(B0), B0 b0) {
  return callback_t<R(void)>(new function_binder_1_t<R, B0>(invoker, b0));
}

template <typename R, typename B0>
callback_t<R(void)> new_callback(R (B0::*invoker)(void), B0 *b0) {
  return callback_t<R(void)>(new method_binder_1_t<R, B0>(invoker, b0));
}

template <typename R, typename B0, typename B1>
callback_t<R(void)> new_callback(R (*invoker)(B0, B1), B0 b0, B1 b1) {
  return callback_t<R(void)>(new function_binder_2_t<R, B0, B1>(invoker, b0, b1));
}

template <typename R, typename B0, typename B1>
callback_t<R(void)> new_callback(R (B0::*invoker)(B1), B0 *b0, B1 b1) {
  return callback_t<R(void)>(new method_binder_2_t<R, B0, B1>(invoker, b0, b1));
}

template <typename R, typename B0, typename B1, typename B2>
callback_t<R(void)> new_callback(R (*invoker)(B0, B1, B2), B0 b0, B1 b1, B2 b2) {
  return callback_t<R(void)>(new function_binder_3_t<R, B0, B1, B2>(invoker, b0, b1, b2));
}

template <typename R, typename B0, typename B1, typename B2>
callback_t<R(void)> new_callback(R (B0::*invoker)(B1), B0 *b0, B1 b1, B2 b2) {
  return callback_t<R(void)>(new method_binder_3_t<R, B0, B1, B2>(invoker, b0, b1, b2));
}

template <typename R, typename A0>
class callback_t<R(A0)> : public abstract_callback_t {
public:
  // This is mainly used for consistency and validation: the binder used by this
  // callback should be of this type.
  typedef binder_t<R, A0> my_binder_t;

  callback_t() : abstract_callback_t() { }

  R operator()(A0 a0) {
    return (static_cast<my_binder_t*>(binder_))->call(a0);
  }

private:
  template <typename FR, typename FA0>
  friend callback_t<FR(FA0)> new_callback(FR (*)(FA0));
  template <typename FR, typename FA0, typename FB0>
  friend callback_t<FR(FA0)> new_callback(FR (*)(FB0, FA0), FB0);
  template <typename FR, typename FA0, typename FB0>
  friend callback_t<FR(FA0)> new_callback(FR (FB0::*)(FA0), FB0*);
  callback_t(my_binder_t *binder) : abstract_callback_t(binder) { }
};

template <typename R, typename A0>
callback_t<R(A0)> new_callback(R (*invoker)(A0)) {
  return callback_t<R(A0)>(new function_binder_0_t<R, A0>(invoker));
}

template <typename R, typename A0, typename B0>
callback_t<R(A0)> new_callback(R (*invoker)(B0, A0), B0 b0) {
  return callback_t<R(A0)>(new function_binder_1_t<R, B0, A0>(invoker, b0));
}

template <typename R, typename A0, typename B0>
callback_t<R(A0)> new_callback(R (B0::*invoker)(A0), B0 *b0) {
  return callback_t<R(A0)>(new method_binder_1_t<R, B0, A0>(invoker, b0));
}

template <typename R, typename A0>
class callback_t<R(A0::*)(void)> : public abstract_callback_t {
public:
  typedef binder_t<R, A0*> my_binder_t;

  callback_t() : abstract_callback_t() { }

  R operator()(A0 *a0) {
    return (static_cast<my_binder_t*>(binder_))->call(a0);
  }

private:
  template <typename FR, typename FA0>
  friend callback_t<FR(FA0::*)(void)> new_callback(FR (FA0::*)(void));
  callback_t(my_binder_t *binder) : abstract_callback_t(binder) { }
};

template <typename R, typename A0>
callback_t<R(A0::*)(void)> new_callback(R (A0::*invoker)(void)) {
  return callback_t<R(A0::*)(void)>(new method_binder_0_t<R, A0>(invoker));
}

template <typename R, typename A0, typename A1>
class callback_t<R(A0, A1)> : public abstract_callback_t {
public:
  // This is mainly used for consistency and validation: the binder used by this
  // callback should be of this type.
  typedef binder_t<R, A0, A1> my_binder_t;

  callback_t() : abstract_callback_t() { }

  R operator()(A0 a0, A1 a1) {
    return (static_cast<my_binder_t*>(binder_))->call(a0, a1);
  }

private:
  template <typename FR, typename FA0, typename FA1>
  friend callback_t<FR(FA0, FA1)> new_callback(FR (*)(FA0, FA1));
  callback_t(my_binder_t *binder) : abstract_callback_t(binder) { }
};

template <typename R, typename A0, typename A1>
callback_t<R(A0, A1)> new_callback(R (*invoker)(A0, A1)) {
  return callback_t<R(A0, A1)>(new function_binder_0_t<R, A0, A1>(invoker));
}

template <typename R, typename A0, typename A1>
class callback_t<R(A0::*)(A1)> : public abstract_callback_t {
public:
  typedef binder_t<R, A0*, A1> my_binder_t;

  callback_t() : abstract_callback_t() { }

  R operator()(A0 *a0, A1 a1) {
    return (static_cast<my_binder_t*>(binder_))->call(a0, a1);
  }

private:
  template <typename FR, typename FA0, typename FA1>
  friend callback_t<FR(FA0::*)(FA1)> new_callback(FR (FA0::*)(FA1));
  callback_t(my_binder_t *binder) : abstract_callback_t(binder) { }
};

template <typename R, typename A0, typename A1>
callback_t<R(A0::*)(A1)> new_callback(R (A0::*invoker)(A1)) {
  return callback_t<R(A0::*)(A1)>(new method_binder_0_t<R, A0, A1>(invoker));
}

} // namespace tclib

#endif // _TCLIB_CALLBACK_HH
