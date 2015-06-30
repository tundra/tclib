//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

#include "sync/intex.hh"
#include "sync/thread.hh"

BEGIN_C_INCLUDES
#include "sync/atomic.h"
END_C_INCLUDES

using namespace tclib;

TEST(atomic, simple) {
  atomic_int32_t atomic = atomic_int32_new(0);
  ASSERT_EQ(0, atomic_int32_get(&atomic));
  ASSERT_EQ(1, atomic_int32_increment(&atomic));
  ASSERT_EQ(1, atomic_int32_get(&atomic));
  ASSERT_EQ(2, atomic_int32_increment(&atomic));
  ASSERT_EQ(2, atomic_int32_get(&atomic));
  ASSERT_EQ(1, atomic_int32_decrement(&atomic));
  ASSERT_EQ(1, atomic_int32_get(&atomic));
}

static void *hammer_counter(Drawbridge *start, atomic_int32_t *count) {
  ASSERT_TRUE(start->pass());
  for (size_t i = 0; i < 65536; i++) {
    if ((i & 1) == 0) {
      atomic_int32_increment(count);
    } else {
      atomic_int32_decrement(count);
    }
  }
  return NULL;
}

#define kThreadCount 16

TEST(atomic, contended) {
  atomic_int32_t counter = atomic_int32_new(0);
  Drawbridge start;
  ASSERT_TRUE(start.initialize());
  NativeThread threads[kThreadCount];
  for (size_t i = 0; i < kThreadCount; i++) {
    threads[i].set_callback(new_callback(hammer_counter, &start, &counter));
    ASSERT_TRUE(threads[i].start());
  }

  ASSERT_TRUE(NativeThread::sleep(Duration::millis(10)));

  ASSERT_TRUE(start.lower());

  for (size_t i = 0; i < kThreadCount; i++)
    threads[i].join();

  ASSERT_EQ(0, atomic_int32_get(&counter));
}
