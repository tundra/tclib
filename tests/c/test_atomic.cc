//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

#include "sync/intex.hh"
#include "sync/thread.hh"

BEGIN_C_INCLUDES
#include "sync/atomic.h"
END_C_INCLUDES

using namespace tclib;

// Wrappers around the atomic types that allow them to be used generically so
// the same tests can be used for either width.
class A32 {
public:
  typedef atomic_int32_t atomic_t;
  static atomic_t init(int32_t v) { return atomic_int32_new(v); }
  static int32_t get(atomic_t *v) { return atomic_int32_get(v); }
  static int32_t inc(atomic_t *v) { return atomic_int32_increment(v); }
  static int32_t dec(atomic_t *v) { return atomic_int32_decrement(v); }
  static int32_t add(atomic_t *v, int32_t d) { return atomic_int32_add(v, d); }
  static int32_t sub(atomic_t *v, int32_t d) { return atomic_int32_subtract(v, d); }
};

class A64 {
public:
  typedef atomic_int64_t atomic_t;
  static atomic_t init(int64_t v) { return atomic_int64_new(v); }
  static int64_t get(atomic_t *v) { return atomic_int64_get(v); }
  static int64_t inc(atomic_t *v) { return atomic_int64_increment(v); }
  static int64_t dec(atomic_t *v) { return atomic_int64_decrement(v); }
  static int64_t add(atomic_t *v, int64_t d) { return atomic_int64_add(v, d); }
  static int64_t sub(atomic_t *v, int64_t d) { return atomic_int64_subtract(v, d); }
};

template <typename A>
static void test_simple() {
  typename A::atomic_t atomic = A::init(0);
  ASSERT_EQ(0, A::get(&atomic));
  ASSERT_EQ(1, A::inc(&atomic));
  ASSERT_EQ(1, A::get(&atomic));
  ASSERT_EQ(2, A::inc(&atomic));
  ASSERT_EQ(2, A::get(&atomic));
  ASSERT_EQ(1, A::dec(&atomic));
  ASSERT_EQ(1, A::get(&atomic));
  ASSERT_EQ(101, A::add(&atomic, 100));
  ASSERT_EQ(101, A::get(&atomic));
  ASSERT_EQ(51, A::sub(&atomic, 50));
  ASSERT_EQ(51, A::get(&atomic));
}

TEST(atomic, simple32) {
  test_simple<A32>();
}

TEST(atomic, simple64) {
  test_simple<A64>();
}

#define kThreadCount 16

template <typename A>
static void *hammer_counter(Drawbridge *start, typename A::atomic_t *count) {
  ASSERT_TRUE(start->pass());
  for (size_t i = 0; i < 65536; i++) {
    if ((i & 1) == 0) {
      A::inc(count);
    } else {
      A::dec(count);
    }
  }
  return NULL;
}

template <typename A>
static void test_contended() {
  typename A::atomic_t counter = A::init(0);
  Drawbridge start;
  ASSERT_TRUE(start.initialize());
  NativeThread threads[kThreadCount];
  for (size_t i = 0; i < kThreadCount; i++) {
    threads[i].set_callback(new_callback(hammer_counter<A>, &start, &counter));
    ASSERT_TRUE(threads[i].start());
  }

  ASSERT_TRUE(NativeThread::sleep(Duration::millis(10)));

  ASSERT_TRUE(start.lower());

  for (size_t i = 0; i < kThreadCount; i++)
    threads[i].join();

  ASSERT_EQ(0, A::get(&counter));
}

TEST(atomic, contended32) {
  test_contended<A32>();
}

TEST(atomic, contended64) {
  test_contended<A64>();
}
