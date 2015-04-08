//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "sync/semaphore.hh"
#include "sync/thread.hh"
#include "sync/mutex.hh"

using namespace tclib;

static void *fail_to_unlock(NativeMutex *m0, NativeMutex *m1) {
  ASSERT_FALSE(m0->try_lock());
  ASSERT_FALSE(NativeMutex::checks_consistency() && m0->unlock());
  ASSERT_FALSE(m1->try_lock());
  ASSERT_FALSE(NativeMutex::checks_consistency() && m1->unlock());
  return NULL;
}

static void *do_lock(NativeMutex *m0, NativeMutex *m1) {
  ASSERT_TRUE(m0->lock());
  ASSERT_TRUE(m1->lock());
  ASSERT_TRUE(m0->unlock());
  ASSERT_TRUE(m1->unlock());
  return NULL;
}

TEST(mutex_cpp, simple) {
  // Some of the failures cause log messages to be issued, which we don't want
  // to see.
  log_o *noisy_log = silence_global_log();

  NativeMutex m0;
  ASSERT_TRUE(m0.initialize());
  NativeMutex m1;
  ASSERT_TRUE(m1.initialize());
  ASSERT_FALSE(NativeMutex::checks_consistency() && m0.unlock());
  ASSERT_TRUE(m0.lock());
  ASSERT_FALSE(NativeMutex::checks_consistency() && m1.unlock());
  ASSERT_TRUE(m1.lock());

  // Try and fail to lock/unlock from a different thread.
  NativeThread failer(new_callback(fail_to_unlock, &m0, &m1));
  ASSERT_TRUE(failer.start());
  failer.join();

  // Locking recursively works.
  ASSERT_TRUE(m1.lock());
  ASSERT_TRUE(m1.lock());
  ASSERT_TRUE(m1.unlock());
  ASSERT_TRUE(m1.unlock());
  ASSERT_TRUE(m1.unlock());
  ASSERT_FALSE(NativeMutex::checks_consistency() && m1.unlock());

  // Unlock completely and let a different thread try locking.
  ASSERT_TRUE(m0.unlock());
  NativeThread succeeder(new_callback(do_lock, &m0, &m1));
  ASSERT_TRUE(succeeder.start());
  succeeder.join();

  set_global_log(noisy_log);
}

#define kBarrierCount 128

static void *run_thread(NativeSemaphore *locked, NativeMutex *mutexes, size_t *order) {
  // Lock its own mutex.
  ASSERT_TRUE(mutexes[0].lock());
  // Signal to the main thread that it's done that.
  ASSERT_TRUE(locked->release());
  // Wait for the previous mutex.
  ASSERT_TRUE(mutexes[-1].lock());
  // Once it has been released, note the order in its field.
  order[0] = order[-1] + 1;
  // Then it's done and can release its own mutex.
  ASSERT_TRUE(mutexes[0].unlock());
  // Remember to also unlock the previous mutex which this thread now holds.
  ASSERT_TRUE(mutexes[-1].unlock());
  return 0;
}

TEST(mutex_cpp, barriers) {
  NativeSemaphore locked_count(0);
  ASSERT_TRUE(locked_count.initialize());
  // Create a vector of locked mutexes.
  NativeMutex mutexes[kBarrierCount];
  for (size_t i = 0; i < kBarrierCount; i++)
    ASSERT_TRUE(mutexes[i].initialize());
  ASSERT_TRUE(mutexes[0].lock());
  // Create a vector of threads that run run_thread and start them up. Each will
  // take its own mutex and then try to take the previous one, which will block
  // it because it will be held by the previous thread.
  NativeThread threads[kBarrierCount];
  size_t order[kBarrierCount];
  memset(order, 0, sizeof(size_t) * kBarrierCount);
  for (size_t i = 1; i < kBarrierCount; i++) {
    threads[i].set_callback(new_callback(run_thread, &locked_count, mutexes + i,
        order + i));
    // Start the thread and then wait for it to release the locked_count.
    ASSERT_FALSE(locked_count.try_acquire());
    ASSERT_TRUE(threads[i].start());
    ASSERT_TRUE(locked_count.acquire());
  }
  // Check that the order hasn't been touched yet because it's guarded by the
  // mutexes.
  for (size_t i = 0; i < kBarrierCount; i++)
    ASSERT_EQ(0, order[i]);
  // Unlock the first mutex. This will start the threads running in a cascade.
  ASSERT_TRUE(mutexes[0].unlock());
  // Wait for the last thread to finish by waiting for the mutex that'll be
  // unlocked by it.
  threads[kBarrierCount - 1].join();
  for (size_t i = 0; i < kBarrierCount; i++)
    ASSERT_EQ(i, order[i]);
  // Remember to join the remaining threads.
  for (size_t i = 1; i < kBarrierCount - 1; i++)
    threads[i].join();
}

#if defined(IS_MSVC)
#  include "c/winhdr.h"
#endif

TEST(mutex_cpp, msvc_sizes) {
#if defined(IS_MSVC)
  // If this fails it should be easy to fix, just bump up the size of the
  // platform mutex type.
  ASSERT_REL(sizeof(platform_mutex_t), >=, sizeof(CRITICAL_SECTION));
#endif
}
