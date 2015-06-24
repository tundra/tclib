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

bool Intex::lock() {
  return guard_.lock();
}

bool Intex::unlock() {
  return guard_.unlock();
}
