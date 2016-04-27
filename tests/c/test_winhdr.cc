//- Copyright 2016 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/stdc.h"

#ifdef IS_MSVC
#  include "c/winhdr.h"
#endif

#ifdef ERROR
#  error "ERROR shouldn't be defined"
#endif

#include "test/unittest.hh"

// There has to be at least one test.
TEST(winhdr, anything) { }
