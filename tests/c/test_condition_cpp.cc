//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "sync/condition.hh"
#include "sync/mutex.hh"
#include "sync/semaphore.hh"
#include "sync/thread.hh"
#include "test/unittest.hh"

using namespace tclib;

TEST(condition_cpp, simple) {
  NativeCondition cond;
  ASSERT_TRUE(cond.initialize());
}

// State shared between all the wake-all waiters.
class WakeAllShared {
public:
  int step;
  NativeMutex mutex;
  NativeCondition cond;
  NativeSemaphore started;
  NativeSemaphore released;
};

// Individual state for a wake-all waiter.
class WakeAllWaiter {
public:
  void start(WakeAllShared *shared, int index);
  void join();
  void *run();
private:
  NativeThread thread_;
  WakeAllShared *shared_;
  volatile int index_;
};

void WakeAllWaiter::start(WakeAllShared *shared, int index) {
  shared_ = shared;
  index_ = index;
  thread_.set_callback(new_callback(&WakeAllWaiter::run, this));
  ASSERT_TRUE(thread_.start());
}

void WakeAllWaiter::join() {
  thread_.join();
}

void *WakeAllWaiter::run() {
  // Enter the mutex and then spin on the condition variable until step gets
  // this waiter's value.
  ASSERT_TRUE(shared_->mutex.lock());
  ASSERT_TRUE(shared_->started.release());
  while (shared_->step != index_)
    ASSERT_TRUE(shared_->cond.wait(&shared_->mutex));
  ASSERT_TRUE(shared_->mutex.unlock());
  ASSERT_TRUE(shared_->released.release());
  return NULL;
}

TEST(condition_cpp, wake_all) {
#define kWaiterCount 16
  WakeAllShared shared;
  shared.step = -1;
  shared.started.set_initial_count(0);
  shared.released.set_initial_count(0);
  ASSERT_TRUE(shared.mutex.initialize());
  ASSERT_TRUE(shared.cond.initialize());
  ASSERT_TRUE(shared.released.initialize());
  ASSERT_TRUE(shared.started.initialize());
  WakeAllWaiter waiters[kWaiterCount];
  for (int i = 0; i < kWaiterCount; i++) {
    // Start a waiter waiting, block until it's within the mutex.
    waiters[i].start(&shared, i);
    ASSERT_TRUE(shared.started.acquire());
  }
  for (int i = 0; i < kWaiterCount; i++) {
    // Jump to the next step, wake all the waiters and observe that this causes
    // one of them to be released.
    ASSERT_TRUE(shared.mutex.lock());
    shared.step = i;
    ASSERT_TRUE(shared.cond.wake_all());
    ASSERT_TRUE(shared.mutex.unlock());
    ASSERT_TRUE(shared.released.acquire());
    // Try unlocking all the conditions again with the same step and observe
    // that none of them get released this time. There is a race condition here
    // in the case where one does get released but doesn't start until after
    // this step but that may be caught by a later iteration.
    ASSERT_TRUE(shared.mutex.lock());
    ASSERT_TRUE(shared.cond.wake_all());
    ASSERT_TRUE(shared.mutex.unlock());
    ASSERT_FALSE(shared.released.acquire(Duration::millis(10)));
  }
  for (size_t i = 0; i < kWaiterCount; i++)
    waiters[i].join();
}

#if defined(IS_MSVC)
#  include "c/winhdr.h"
#endif

TEST(condition_cpp, msvc_sizes) {
#if defined(IS_MSVC)
  // If this fails it should be easy to fix, just bump up the size of the
  // platform condition type.
  ASSERT_REL(sizeof(platform_condition_t), >=, sizeof(CONDITION_VARIABLE));
#endif
}
