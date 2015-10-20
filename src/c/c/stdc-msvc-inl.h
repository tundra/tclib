//- Copyright 2013 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <limits>

// MSVC is either missing the standard definition of infinity, or if it does
// have one it causes warnings to be issues randomly. So instead we do this.

#undef kFloatInfinity
#define kFloatInfinity std::numeric_limits<float>::infinity()

#ifndef NAN
#  define NAN std::numeric_limits<float>::quiet_NaN()
#endif

#define isfinite(V) _finite(V)

#define isnan(V) _isnan(V)
