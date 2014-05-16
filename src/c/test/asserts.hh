//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

/// Basic C assertions for testing. These should not be used in program code,
/// only tests.

#ifndef _ASSERTS
#define _ASSERTS

#include "../stdc.h"

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

#endif // _ASSERTS
