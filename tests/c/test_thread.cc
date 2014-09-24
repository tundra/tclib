//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "sync/thread.hh"

BEGIN_C_INCLUDES
#include "sync/thread.h"
END_C_INCLUDES

using namespace tclib;

class CallCounter {
public:
  CallCounter() : value(0) { }
  void *run();
  int value;
};

void *CallCounter::run() {
  value++;
  return static_cast<void*>(this);
}

TEST(thread, simple_cpp) {
  CallCounter counter;
  NativeThread thread(new_callback(&CallCounter::run, &counter));
  ASSERT_EQ(0, counter.value);
  ASSERT_TRUE(thread.start());
  ASSERT_PTREQ(&counter, thread.join());
  ASSERT_EQ(1, counter.value);
}

static void *run_call_counter_bridge(void *data) {
  CallCounter *self = static_cast<CallCounter*>(data);
  return self->run();
}

TEST(thread, simple_c) {
  CallCounter counter;
  native_thread_t *thread = new_native_thread(run_call_counter_bridge, &counter);
  ASSERT_EQ(0, counter.value);
  ASSERT_TRUE(native_thread_start(thread));
  ASSERT_PTREQ(&counter, native_thread_join(thread));
  ASSERT_EQ(1, counter.value);
  dispose_native_thread(thread);
}

void *check_thread_not_equal(native_thread_id_t that) {
  native_thread_id_t own = NativeThread::get_current_id();
  ASSERT_TRUE(NativeThread::ids_equal(own, own));
  ASSERT_FALSE(NativeThread::ids_equal(own, that));
  return NULL;
}

TEST(thread, cpp_equality) {
  native_thread_id_t current = NativeThread::get_current_id();
  ASSERT_TRUE(NativeThread::ids_equal(current, current));
  ASSERT_TRUE(NativeThread::ids_equal(NativeThread::get_current_id(),
      NativeThread::get_current_id()));
  NativeThread thread(new_callback(&check_thread_not_equal, current));
  thread.start();
  thread.join();
}
