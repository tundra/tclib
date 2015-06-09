//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

/// A few abstractions for working with time.

#ifndef _UTILS_DURATION_HH
#define _UTILS_DURATION_HH

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "sync/sync.h"
#include "utils/duration.h"
END_C_INCLUDES

#if !defined(IS_MSVC)
#  include <time.h>
#endif

namespace tclib {

class Duration : public duration_t {
public:
  Duration(const duration_t &that) { *static_cast<duration_t*>(this) = that; }

  // Returns the number of milliseconds of this duration. If the given duration
  // is unlimited or instant 0 will be returned (which means that you probably
  // want to test explicitly for unlimited).
  uint64_t to_millis() { return duration_to_millis(*this); }

  // Is this value the unlimited duration?
  bool is_unlimited() { return duration_is_unlimited(*this); }

  // Is this duration the instant duration, that is, zero time?
  bool is_instant() { return duration_is_instant(*this); }

  // Returns the unlimited duration.
  static Duration unlimited() { return duration_unlimited(); }

  // Returns the instant duration, that is, zero time.
  static Duration instant() { return duration_instant(); }

  // Returns a duration that represents the given number of milliseconds.
  static Duration millis(uint64_t value) { return duration_millis(value); }

  // Returns a duration that represents the given number of seconds.
  static Duration seconds(double value) { return duration_seconds(value); }
};

} // namespace tclib

#endif // _UTILS_DURATION_HH

