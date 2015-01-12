//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "sync/thread.hh"
#include "sync/semaphore.hh"

using namespace tclib;

TEST(semaphore, default_initial) {
  NativeSemaphore sema;
  ASSERT_TRUE(sema.initialize());
  ASSERT_TRUE(sema.acquire());
  ASSERT_FALSE(sema.try_acquire());
  ASSERT_TRUE(sema.release());
  ASSERT_TRUE(sema.acquire());
  ASSERT_FALSE(sema.try_acquire());
  ASSERT_TRUE(sema.release());
  ASSERT_TRUE(sema.release());
  ASSERT_TRUE(sema.try_acquire());
  ASSERT_TRUE(sema.try_acquire());
  ASSERT_FALSE(sema.try_acquire());
}

TEST(semaphore, explicit_initial) {
  NativeSemaphore sema(256);
  ASSERT_TRUE(sema.initialize());
  for (size_t i = 0; i < 256; i++)
    ASSERT_TRUE(sema.acquire());
  ASSERT_FALSE(sema.try_acquire());
}

static const size_t kThreadCount = 128;

// Run an individual waiter thread.
static void *run_waiter(NativeSemaphore *started_count,
    NativeSemaphore *released_count, NativeSemaphore *done_count) {
  ASSERT_TRUE(started_count->release());
  ASSERT_TRUE(released_count->acquire());
  ASSERT_TRUE(done_count->release());
  return NULL;
}

// Wait for all the threads to join, then release the all_joined semaphore,
// then leave.
static void *run_join_monitor(NativeSemaphore *all_joined, NativeThread *threads) {
  for (size_t i = 0; i < kThreadCount; i++)
    threads[i].join();
  all_joined->release();
  return NULL;
}

// This test starts up a number of threads that first release the started_count
// semaphore to signal that they're running, then all wait on the same empty
// semaphore, released_count, which gets fed permits one at a time. Once a
// thread has acquired a release_count permit it releases a done_count itself.
// In addition the join_monitor thread waits for all the threads to join and
// then release the all_joined sema.
TEST(semaphore, waiters) {
  // Initialize semas.
  NativeSemaphore started_count(0);
  NativeSemaphore released_count(0);
  NativeSemaphore done_count(0);
  NativeSemaphore all_joined(0);
  ASSERT_TRUE(started_count.initialize());
  ASSERT_TRUE(released_count.initialize());
  ASSERT_TRUE(done_count.initialize());
  ASSERT_TRUE(all_joined.initialize());
  // Spin off all the waiter threads.
  NativeThread threads[kThreadCount];
  for (size_t i = 0; i < kThreadCount; i++) {
    threads[i].set_callback(new_callback(run_waiter, &started_count,
        &released_count, &done_count));
    ASSERT_TRUE(threads[i].start());
  }
  // Spin off the join monitor.
  NativeThread join_monitor(new_callback(run_join_monitor, &all_joined, threads));
  ASSERT_TRUE(join_monitor.start());
  // Wait for all the threads to have started.
  for (size_t i = 0; i < kThreadCount; i++)
    ASSERT_TRUE(started_count.acquire());
  // At this point they should all be blocked on the release count; the done
  // count should not have changed yet.
  ASSERT_FALSE(done_count.try_acquire());
  // Release permits to release_count one at a time.
  for (size_t i = 0; i < kThreadCount; i++) {
    // All the threads should not have joined yet.
    ASSERT_FALSE(all_joined.try_acquire());
    // Releasing one thread should cause exactly one permit to be released to
    // the done count.
    ASSERT_TRUE(released_count.release());
    ASSERT_TRUE(done_count.acquire());
    ASSERT_FALSE(done_count.try_acquire());
    // The released count should have been consumed by the waiter.
    ASSERT_FALSE(released_count.try_acquire());
  }
  // Now all the waiters have been released so they should join.
  ASSERT_TRUE(all_joined.acquire());
  join_monitor.join();
}

TEST(semaphore, timed_wait) {
  NativeSemaphore sema(0);
  ASSERT_TRUE(sema.initialize());
  ASSERT_FALSE(sema.acquire(duration_millis(100)));
}
