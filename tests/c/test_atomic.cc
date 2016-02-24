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

MULTITEST(atomic, simple, typename, ("32", A32), ("64", A64)) {
  typedef Flavor A;
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

#define kThreadCount 16

template <typename A>
static opaque_t hammer_counter(Drawbridge *start, typename A::atomic_t *count) {
  ASSERT_TRUE(start->pass());
  for (size_t i = 0; i < 65536; i++) {
    if ((i & 1) == 0) {
      A::inc(count);
    } else {
      A::dec(count);
    }
  }
  return o0();
}

MULTITEST(atomic, contended, typename, ("32", A32), ("64", A64)) {
  typedef Flavor A;
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
    ASSERT_TRUE(threads[i].join(NULL));

  ASSERT_EQ(0, A::get(&counter));
}

static opaque_t hammer_compare_and_set(Drawbridge *start, atomic_int32_t *a,
    int32_t id) {
  ASSERT_TRUE(start->pass());
  // Each thread tries to set the atomic to their id from 0. Each time one
  // succeeds the value must now be their id so setting it back must succeed. If
  // compare_and_set is not thread safe then multiple threads may think they
  // have succeeded and one that actually didn't will fail the test.
  ASSERT_REL(id, >, 0);
  for (size_t i = 0; i < 1024; i++) {
    if (atomic_int32_compare_and_set(a, 0, id))
      ASSERT_TRUE(atomic_int32_compare_and_set(a, id, 0));
  }
  return o0();
}

TEST(atomic, compare_and_set) {
  NativeThread threads[kThreadCount];
  atomic_int32_t a = atomic_int32_new(0);
  Drawbridge start;
  ASSERT_TRUE(start.initialize());
  for (int32_t i = 0; i < kThreadCount; i++) {
    threads[i].set_callback(new_callback(hammer_compare_and_set, &start, &a, i + 1));
    ASSERT_TRUE(threads[i].start());
  }
  ASSERT_TRUE(NativeThread::sleep(Duration::millis(10)));
  ASSERT_TRUE(start.lower());
  for (size_t i = 0; i < kThreadCount; i++)
    ASSERT_TRUE(threads[i].join(NULL));
}
