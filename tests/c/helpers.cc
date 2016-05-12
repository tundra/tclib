//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "helpers.hh"

utf8_t get_durian_main() {
  const char *result = getenv("DURIAN");
  ASSERT_TRUE(result != NULL);
  return new_c_string(result);
}

utf8_t get_injectee_dll() {
  const char *result = getenv("INJECTEE");
  ASSERT_TRUE(result != NULL);
  return new_c_string(result);
}
