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

fat_bool_t Intex::initialize() {
  F_TRY(guard()->initialize());
  F_TRY(cond()->initialize());
  is_initialized_ = true;
  return F_TRUE;
}

Intex::~Intex() {
  if (!is_initialized_)
    return;
  guard()->~NativeMutex();
  cond()->~NativeCondition();
}

fat_bool_t Intex::set(uint64_t value) {
  value_ = value;
  return cond()->wake_all();
}

fat_bool_t Intex::add(int64_t value) {
  return set(value_ + value);
}

fat_bool_t Intex::lock(Duration timeout) {
  return guard()->lock(timeout);
}

fat_bool_t Intex::unlock() {
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

fat_bool_t Drawbridge::initialize() {
  return intex_.initialize();
}

fat_bool_t Drawbridge::lower(Duration timeout) {
  F_TRY(intex_.lock(timeout));
  F_TRY(intex_.set(dsLowered));
  F_TRY(intex_.unlock());
  return F_TRUE;
}

fat_bool_t Drawbridge::raise(Duration timeout) {
  F_TRY(intex_.lock(timeout));
  F_TRY(intex_.set(dsRaised));
  F_TRY(intex_.unlock());
  return F_TRUE;
}

fat_bool_t Drawbridge::pass(Duration timeout) {
  F_TRY(intex_.lock_when(timeout) == dsLowered);
  F_TRY(intex_.unlock());
  return F_TRUE;
}
