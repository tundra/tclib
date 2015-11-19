#include "c/stdc.h"

#include "test/unittest.hh"
#include "helpers.hh"

utf8_t get_durian_main() {
  const char *result = getenv("DURIAN_MAIN");
  ASSERT_TRUE(result != NULL);
  return new_c_string(result);
}

utf8_t get_injectee_dll() {
  const char *result = getenv("INJECTEE");
  ASSERT_TRUE(result != NULL);
  return new_c_string(result);
}
