//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "sync/intex.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
END_C_INCLUDES

#include <new>

using namespace tclib;

Intex::Intex(uint64_t init_value) {
  value_ = init_value;
  new (guard()) NativeMutex();
  new (cond()) NativeCondition();
  is_initialized_ = false;
}

bool Intex::initialize() {
  return is_initialized_ = (guard()->initialize() && cond()->initialize());
}

Intex::~Intex() {
  if (!is_initialized_)
    return;
  guard()->~NativeMutex();
  cond()->~NativeCondition();
}

bool Intex::set(uint64_t value) {
  value_ = value;
  return cond()->wake_all();
}

bool Intex::lock(Duration timeout) {
  return guard()->lock(timeout);
}

bool Intex::unlock() {
  return guard()->unlock();
}

void intex_construct(intex_t *intex, uint64_t init_value) {
  new (intex) Intex(init_value);
}

void intex_dispose(intex_t *intex) {
  static_cast<Intex*>(intex)->~Intex();
}

bool intex_initialize(intex_t *intex) {
  return static_cast<Intex*>(intex)->initialize();
}

bool intex_lock_when_equal(intex_t *intex, int64_t value, duration_t timeout) {
  return static_cast<Intex*>(intex)->lock_when(Duration(timeout)) == value;
}

bool intex_lock_when_less(intex_t *intex, int64_t value, duration_t timeout) {
  return static_cast<Intex*>(intex)->lock_when(Duration(timeout)) < value;
}

bool intex_lock_when_greater(intex_t *intex, int64_t value, duration_t timeout) {
  return static_cast<Intex*>(intex)->lock_when(Duration(timeout)) > value;
}

bool intex_unlock(intex_t *intex) {
  return static_cast<Intex*>(intex)->unlock();
}

Drawbridge::Drawbridge(state_t state)
  : intex_(state) { }

bool Drawbridge::initialize() {
  return intex_.initialize();
}

bool Drawbridge::lower(Duration timeout) {
  return intex_.lock(timeout) && intex_.set(dsLowered) && intex_.unlock();
}

bool Drawbridge::raise(Duration timeout) {
  return intex_.lock(timeout) && intex_.set(dsRaised) && intex_.unlock();
}

bool Drawbridge::pass(Duration timeout) {
  return (intex_.lock_when(timeout) == dsLowered) && intex_.unlock();
}
