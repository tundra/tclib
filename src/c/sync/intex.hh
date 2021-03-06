//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_INTEX_HH
#define _TCLIB_INTEX_HH

#include "c/stdc.h"

#include "sync/condition.hh"
#include "sync/mutex.hh"

BEGIN_C_INCLUDES
#include "sync/intex.h"
END_C_INCLUDES

namespace tclib {

class Intex;
class IntexLocker;

// An intex is like a mutex except that in addition to the standard lock and
// unlock behavior it can also be locked conditionally on an integer value. So
// a thread will not only wait for the lock to become available but for the
// intex to reach a particular value or range of values.
class Intex : public intex_t {
public:
  // Construct this intex with the given initial value. Note that before use
  // the intex has to be explicitly initialized.
  explicit Intex(uint64_t init_value = 0);

  ~Intex();

  // Initializes the state of this intex, returning true if initialization
  // succeeded.
  fat_bool_t initialize();

  // Sets the value of this intex and, if at least one thread is currently
  // blocked waiting for this intex that can be released with the new value,
  // causes that thread to be given a chance to be released. It may not
  // ultimately be released though because other threads (or this one) may
  // change the value further in the meantime.
  //
  // The intex must currently be locked by the calling thread.
  fat_bool_t set(uint64_t value);

  // Adds the given, possibly negative, delta to the value of this intex. Uses
  // ::set so all the same conditions apply.
  fat_bool_t add(int64_t delta);

  // Wait for this intex to become available, then acquires it regardless of
  // its value.
  fat_bool_t lock(Duration timeout = Duration::unlimited());

  // Releases this intex which must already be held.
  fat_bool_t unlock();

private:
  // Classes that implement the different operators so they can be passed as
  // template parameters to lock_cond by the dispatcher.
  struct Eq { static bool eval(uint64_t a, uint64_t b) { return a == b; } };
  struct Neq { static bool eval(uint64_t a, uint64_t b) { return a != b; } };
  struct Lt { static bool eval(uint64_t a, uint64_t b) { return a < b; } };
  struct Leq { static bool eval(uint64_t a, uint64_t b) { return a <= b; } };
  struct Gt { static bool eval(uint64_t a, uint64_t b) { return a > b; } };
  struct Geq { static bool eval(uint64_t a, uint64_t b) { return a >= b; } };

  // Lock this intex conditionally on the type of condition given as a template
  // argument.
  template <typename C>
  fat_bool_t lock_cond(Duration timeout, uint64_t target);

  NativeMutex *guard() { return static_cast<NativeMutex*>(&guard_); }

  NativeCondition *cond() { return static_cast<NativeCondition*>(&cond_); }

  class Dispatcher {
  public:
    // Wait for the value to become equal to another value.
    fat_bool_t operator==(uint64_t value) { return intex_->lock_cond<Eq>(timeout_, value); }

    // Wait for the value to become different from another value.
    fat_bool_t operator!=(uint64_t value) { return intex_->lock_cond<Neq>(timeout_, value); }

    // Wait for the value to become less than another value.
    fat_bool_t operator<(uint64_t value) { return intex_->lock_cond<Lt>(timeout_, value); }

    // Wait for the value to become less than or equal to another value.
    fat_bool_t operator<=(uint64_t value) { return intex_->lock_cond<Leq>(timeout_, value); }

    // Wait for the value to become greater than another value.
    fat_bool_t operator>(uint64_t value) { return intex_->lock_cond<Gt>(timeout_, value); }

    // Wait for the value to become greater than or equal to another value.
    fat_bool_t operator>=(uint64_t value) { return intex_->lock_cond<Geq>(timeout_, value); }

    explicit Dispatcher(Intex *intex, Duration timeout)
      : intex_(intex)
      , timeout_(timeout) { }

    Intex *intex_;
    Duration timeout_;
  };

public:
  // Lock this intex when the value reaches a particular value or range of
  // values. For instance, to wait for the value to become greater than 3 you
  // would do,
  //
  //   intex.lock_when() > 3;
  //
  // This involves a bit of operator overloading magic but the assumption is
  // that having the actual relation you're waiting for is less error prone than
  // having to map the operators to different alnum-named methods.
  inline Dispatcher lock_when(Duration timeout = Duration::unlimited()) {
    return Dispatcher(this, timeout);
  }
};

template <typename C>
fat_bool_t Intex::lock_cond(Duration timeout, uint64_t target) {
  F_TRY(guard()->lock(timeout));
  // We now have to lock, now spin around waiting for the value to become what
  // we're waiting for.
  while (!C::eval(value_, target)) {
    fat_bool_t waited = cond()->wait(guard(), timeout);
    if (!waited) {
      guard()->unlock();
      return waited;
    }
  }
  return F_TRUE;
}

// A drawbridge is a simple wrapper around an intex. It allows threads to be
// blocked on a boolean condition: whether the bridge is lowered or raised.
// While it is raised callers to pass will block and wait until it gets lowered.
class Drawbridge {
public:
  enum state_t {
    dsRaised,
    dsLowered
  };

  // Construct this drawbridge. Note that initialize must be called before it
  // can be used. By default a drawbridge starts out raised.
  Drawbridge(state_t init_state = dsRaised);

  // Initialize the internal state of this drawbridge.
  fat_bool_t initialize();

  // Block this drawbridge so no more threads can pass it. Raising an already
  // raised drawbridge has no effect.
  fat_bool_t raise(Duration timeout = Duration::unlimited());

  // Unblock this drawbridge so threads are free to pass it. Lowering an already
  // lowered drawbridge has no effect.
  fat_bool_t lower(Duration timeout = Duration::unlimited());

  // Wait for this drawbridge to be lowered, then pass it.
  fat_bool_t pass(Duration timeout = Duration::unlimited());

private:
  Intex intex_;
};

} // namespace tclib

#endif // _TCLIB_INTEX_HH
