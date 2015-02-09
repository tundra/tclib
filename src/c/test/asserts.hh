//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

/// Basic C assertions for testing. These should not be used in program code,
/// only tests.

#ifndef _ASSERTS
#define _ASSERTS

#include "../c/stdc.h"

// Aborts exception, signalling an error. The test library doesn't implement
// this, any code that uses this has to provide their own implementation that
// fails in some appropriate way.
void fail(const char *file, int line, const char *fmt, ...);

// Fails unless the two values are equal.
#define ASSERT_EQ(A, B) do {                                                   \
  int64_t __a__ = (int64_t) (A);                                               \
  int64_t __b__ = (int64_t) (B);                                               \
  if (__a__ != __b__)                                                          \
    fail(__FILE__, __LINE__, "Assertion failed: %s == %s.\n  Expected: %lli\n  Found: %lli", \
        #A, #B, __a__, __b__);                                                 \
} while (false)

// Bit-casts a void* to an integer.
static int64_t ptr_to_int_bit_cast(void *value) {
  int64_t result = 0;
  memcpy(&result, &value, sizeof(value));
  return result;
}

// Fails unless the two pointer values are equal.
#define ASSERT_PTREQ(A, B) ASSERT_EQ(ptr_to_int_bit_cast(A), ptr_to_int_bit_cast(B))

// Fails unless the two values are different.
#define ASSERT_NEQ(A, B) ASSERT_FALSE((A) == (B))

// Fails unless the condition is true.
#define ASSERT_TRUE(COND) ASSERT_EQ(COND, true)

// Fails unless the condition is false.
#define ASSERT_FALSE(COND) ASSERT_EQ(COND, false)

// Fails unless the two given strings (string_t*) are equal.
#define ASSERT_STREQ(A, B) do {                                                \
  utf8_t __a__ = (A);                                                          \
  utf8_t __b__ = (B);                                                          \
  if (!(string_equals(__a__, __b__)))                                          \
    fail(__FILE__, __LINE__, "Assertion failed: %s == %s.\n  Expected: %s\n  Found: %s", \
        #A, #B, __a__.chars, __b__.chars);                                     \
} while (false)

// Fails unless the two given c-strings are equal.
#define ASSERT_C_STREQ(A, B) do {                                              \
  utf8_t __sa__ = new_c_string(A);                                             \
  utf8_t __sb__ = new_c_string(B);                                             \
  ASSERT_STREQ(__sa__, __sb__);                                                \
} while (false)

#endif // _ASSERTS
