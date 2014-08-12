//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "sync/thread.hh"

using namespace tclib;

static int call_count = 0;

static void *run_thread() {
  call_count++;
  return static_cast<void*>(&call_count);
}

TEST(thread, simple) {
  NativeThread thread(run_thread);
  thread.start();
  ASSERT_PTREQ(&call_count, thread.join());
  ASSERT_EQ(1, call_count);
}
