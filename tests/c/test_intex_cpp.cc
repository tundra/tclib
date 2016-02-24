//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "sync/intex.hh"
#include "sync/thread.hh"
#include "sync/semaphore.hh"

using namespace tclib;

#define kThreadCount 16

class TestData {
public:
  TestData() : count(0) { }
  Intex intex;
  NativeSemaphore count;
  std::vector<size_t> order;
};

static opaque_t run_simple_thread(TestData *data, size_t value) {
  ASSERT_TRUE(data->intex.lock_when() == value);
  data->order.push_back(value);
  ASSERT_TRUE(data->intex.unlock());
  ASSERT_TRUE(data->count.release());
  return o0();
}

TEST(intex_cpp, simple) {
  TestData data;
  ASSERT_TRUE(data.intex.initialize());
  ASSERT_TRUE(data.count.initialize());

  // Spin off N threads each blocking on different values for the intex.
  NativeThread threads[kThreadCount];
  for (size_t i = 0; i < kThreadCount; i++) {
    threads[i].set_callback(new_callback(run_simple_thread, &data, i + 1));
    ASSERT_TRUE(threads[i].start());
  }

  // Sleep for a little while to give the threads a better chance to start and
  // block.
  ASSERT_TRUE(NativeThread::sleep(Duration::millis(10)));

  for (size_t i = 0; i < kThreadCount; i++) {
    // Acquire the intex, regardless of its value.
    ASSERT_TRUE(data.intex.lock());
    ASSERT_EQ(i, data.order.size());
    // Update the value to let the next waiter run and unlock to let it run.
    ASSERT_TRUE(data.intex.set(i + 1));
    ASSERT_TRUE(data.intex.unlock());
    // Wait for it to finish running.
    ASSERT_TRUE(data.count.acquire());
    // Check that it has actually run.
    ASSERT_EQ(i + 1, data.order.size());
    ASSERT_TRUE(threads[i].join(NULL));
  }

  // Check that the threads were released in the expected order.
  for (size_t i = 0; i < kThreadCount; i++)
    ASSERT_EQ(i + 1, data.order[i]);
}

TEST(intex_cpp, drawbridge_simple) {
  Drawbridge bridge;
  ASSERT_TRUE(bridge.initialize());
  ASSERT_FALSE(bridge.pass(Duration::instant()));
  ASSERT_TRUE(bridge.lower());
  ASSERT_TRUE(bridge.pass(Duration::instant()));
}

static opaque_t run_drawbridge_thread(Drawbridge *bridge, NativeSemaphore *count) {
  ASSERT_TRUE(bridge->pass());
  ASSERT_TRUE(count->release());
  return o0();
}

TEST(intex_cpp, drawbridge_multi) {
  Drawbridge bridge(Drawbridge::dsRaised);
  ASSERT_TRUE(bridge.initialize());
  NativeSemaphore count(0);
  ASSERT_TRUE(count.initialize());

  // Spin off N threads all waiting to pass the drawbridge.
  NativeThread threads[kThreadCount];
  for (size_t i = 0; i < kThreadCount; i++) {
    threads[i].set_callback(new_callback(run_drawbridge_thread, &bridge, &count));
    ASSERT_TRUE(threads[i].start());
  }

  // Wait a while to give them a better chance to block.
  ASSERT_TRUE(NativeThread::sleep(Duration::millis(10)));

  // Check that none of the waiters have passed the bridge yet.
  ASSERT_FALSE(count.acquire(Duration::instant()));

  // Lower and watch them pass.
  ASSERT_TRUE(bridge.lower());
  for (size_t i = 0; i < kThreadCount; i++)
    ASSERT_TRUE(count.acquire());

  for (size_t i = 0; i < kThreadCount; i++)
    ASSERT_TRUE(threads[i].join(NULL));
}
