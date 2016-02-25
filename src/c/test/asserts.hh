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

// Fails unless the given relation holds between the two values.
#define ASSERT_REL(A, REL, B) do {                                             \
  int64_t __a__ = static_cast<int64_t>(A);                                     \
  int64_t __b__ = static_cast<int64_t>(B);                                     \
  if (!(__a__ REL __b__))                                                      \
    fail(__FILE__, __LINE__,                                                   \
        "Assertion failed: " #A " " #REL " " #B ".\n"                          \
        "  Left: %" PRIi64 "\n"                                                \
        "  Right: %" PRIi64 "\n",                                              \
        __a__, __b__);                                                         \
} while (false)

// Works the same way as ASSERT_TRUE but gives more information for fat-bools.
#define ASSERT_F_TRUE(COND) do {                                               \
  fat_bool_t __a__ = (COND);                                                   \
  if (!__a__)                                                                  \
    fail(__FILE__, __LINE__,                                                   \
        "Assertion failed: " #COND ".\n"                                       \
        "  Location: 0x%04x:%i\n",                                             \
        __a__.file_id(), __a__.line());                                        \
} while (false)

// Fails unless the given relation holds between the two values.
#define ASSERT_REL_WITH_HINT(HINT, A, REL, B) do {                             \
  int64_t __a__ = static_cast<int64_t>(A);                                     \
  int64_t __b__ = static_cast<int64_t>(B);                                     \
  int64_t __hint__ = static_cast<int64_t>(HINT);                               \
  if (!(__a__ REL __b__))                                                      \
    fail(__FILE__, __LINE__,                                                   \
        "Assertion failed: " #A " " #REL " " #B ".\n"                          \
        "  Left: %" PRIi64 "\n"                                                \
        "  Right: %" PRIi64 "\n"                                               \
        "  Hint: %" PRIi64,                                                    \
        __a__, __b__, __hint__);                                               \
} while (false)

// Fails unless the two values are equal.
#define ASSERT_EQ(A, B) ASSERT_REL(A, ==, B)

// Bit-casts a void* to an integer.
static int64_t ptr_to_int_bit_cast(void *value) {
  int64_t result = 0;
  memcpy(&result, &value, sizeof(value));
  return result;
}

// Fails unless the two pointer values are equal.
#define ASSERT_PTREQ(A, B) ASSERT_EQ(ptr_to_int_bit_cast(A), ptr_to_int_bit_cast(B))

// Fails unless the two values are different.
#define ASSERT_NEQ(A, B) ASSERT_REL(A, !=, B)

// Fails unless the condition is true.
#define ASSERT_TRUE(COND) ASSERT_EQ(COND, true)

#define ASSERT_TRUE_WITH_HINT(HINT, COND) ASSERT_REL_WITH_HINT((HINT), (COND), ==, true)

// Fails unless the condition is false.
#define ASSERT_FALSE(COND) ASSERT_EQ(COND, false)

// Fails unless the two given strings (string_t*) are equal.
#define ASSERT_STREQ(A, B) do {                                                \
  utf8_t __a__ = (A);                                                          \
  utf8_t __b__ = (B);                                                          \
  if (!(string_equals(__a__, __b__)))                                          \
    fail(__FILE__, __LINE__, "Assertion failed: %s == %s.\n  Expected: [%s]\n  Found: [%s]", \
        #A, #B, __a__.chars, __b__.chars);                                     \
} while (false)

// Fails unless the two given c-strings are equal.
#define ASSERT_C_STREQ(A, B) do {                                              \
  utf8_t __sa__ = new_c_string(A);                                             \
  utf8_t __sb__ = new_c_string(B);                                             \
  ASSERT_STREQ(__sa__, __sb__);                                                \
} while (false)

#endif // _ASSERTS
