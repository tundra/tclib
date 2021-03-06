//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "async/workpool.hh"
#include "test/unittest.hh"

using namespace tclib;

static opaque_t inc_var(int expected, int *var) {
  ASSERT_EQ(expected, *var);
  *var += 1;
  return o0();
}

TEST(workpool_cpp, simple) {
  Workpool pool;
  ASSERT_TRUE(pool.initialize());
  ASSERT_TRUE(pool.start());
  int count = 0;
  ASSERT_TRUE(pool.add_task(new_callback(inc_var, 0, &count), tfRequired));
  ASSERT_TRUE(pool.add_task(new_callback(inc_var, 1, &count), tfRequired));
  ASSERT_TRUE(pool.add_task(new_callback(inc_var, 2, &count), tfRequired));
  ASSERT_TRUE(pool.add_task(new_callback(inc_var, 3, &count), tfRequired));
  ASSERT_TRUE(pool.add_task(new_callback(inc_var, 4, &count), tfRequired));
  ASSERT_TRUE(pool.join());
  ASSERT_EQ(5, count);
}

TEST(workpool_cpp, daemons) {
  Workpool pool;
  ASSERT_TRUE(pool.initialize());
  pool.set_skip_daemons(true);
  int count = 0;
  int daemon_count = 0;
  for (int i = 0; i < 100; i++) {
    ASSERT_TRUE(pool.add_task(new_callback(inc_var, 0, &daemon_count), tfDaemon));
    if ((i % 5) == 0)
      ASSERT_TRUE(pool.add_task(new_callback(inc_var, i / 5, &count), tfRequired));
  }
  ASSERT_TRUE(pool.start());
  ASSERT_TRUE(pool.join(true));
  ASSERT_EQ(20, count);
  ASSERT_EQ(0, daemon_count);
}

static opaque_t release_semaphore(NativeSemaphore *sema) {
  ASSERT_TRUE(sema->release());
  return o0();
}

static opaque_t run_producer(Workpool *pool) {
  NativeSemaphore sema(0);
  ASSERT_TRUE(sema.initialize());
  int count = 0;
  for (int i = 0; i < 2048; i++) {
    if ((i % 256) == 0) {
      // For every 256 tasks we wait for the pool to catch up such that the
      // speeds of the producers are kept similar to the pool such that the
      // chance of contention is increased.
      ASSERT_TRUE(pool->add_task(new_callback(release_semaphore, &sema), tfRequired));
      ASSERT_TRUE(sema.acquire());
      ASSERT_EQ(i, count);
    }
    ASSERT_TRUE(pool->add_task(new_callback(inc_var, i, &count), tfRequired));
  }
  // Synchronize one last time to ensure that we're done accessing the count
  // variable.
  ASSERT_TRUE(pool->add_task(new_callback(release_semaphore, &sema), tfRequired));
  ASSERT_TRUE(sema.acquire());
  ASSERT_EQ(2048, count);
  return o0();
}

#define kProducerCount 8

TEST(workpool_cpp, contended) {
  Workpool pool;
  ASSERT_TRUE(pool.initialize());
  ASSERT_TRUE(pool.start());
  NativeThread producers[kProducerCount];
  for (size_t i = 0; i < kProducerCount; i++) {
    producers[i].set_callback(new_callback(run_producer, &pool));
    ASSERT_TRUE(producers[i].start());
  }
  for (size_t i = 0; i < kProducerCount; i++) {
    opaque_t join_result = o0();
    ASSERT_TRUE(producers[i].join(&join_result));
    ASSERT_PTREQ(NULL, o2p(join_result));
  }
  ASSERT_TRUE(pool.join(false));
}
