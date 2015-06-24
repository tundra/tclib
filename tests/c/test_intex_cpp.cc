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

static void *run_thread(TestData *data, size_t value) {
  ASSERT_TRUE(data->intex.lock_when() == value);
  data->order.push_back(value);
  ASSERT_TRUE(data->intex.unlock());
  ASSERT_TRUE(data->count.release());
  return NULL;
}

TEST(intex_cpp, simple) {
  TestData data;
  ASSERT_TRUE(data.intex.initialize());
  ASSERT_TRUE(data.count.initialize());

  // Spin off N threads each blocking on different values for the intex.
  NativeThread threads[kThreadCount];
  for (size_t i = 0; i < kThreadCount; i++) {
    threads[i].set_callback(new_callback(run_thread, &data, i + 1));
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
    threads[i].join();
  }

  // Check that the threads were released in the expected order.
  for (size_t i = 0; i < kThreadCount; i++)
    ASSERT_EQ(i + 1, data.order[i]);
}
