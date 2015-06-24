//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "sync/intex.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
END_C_INCLUDES

#include <new>

using namespace tclib;

Intex::Intex(uint64_t init_value)
  : value_(init_value) { }

bool Intex::initialize() {
  return guard_.initialize() && cond_.initialize();
}

bool Intex::set(uint64_t value) {
  value_ = value;
  return cond_.wake_all();
}

bool Intex::lock(Duration timeout) {
  return guard_.lock(timeout);
}

bool Intex::unlock() {
  return guard_.unlock();
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
