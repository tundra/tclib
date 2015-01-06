//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

// This test exists to ensure that we have two objects linked together that both
// use the no-allocation constructor for callbacks. This uses data that should
// be shared between binaries, or at least not cause conflicts, so this test is
// considered to pass as long as the test binary links.

#include "test/unittest.hh"
#include "callback.hh"

using namespace tclib;

int return_two() {
  return 2;
}

TEST(callback_link, link_two) {
  callback_t<int(void)> callback = return_two;
  ASSERT_EQ(2, callback());
}
