//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "sync/thread.hh"
#include "utils/clock.hh"

BEGIN_C_INCLUDES
#include "sync/thread.h"
END_C_INCLUDES

using namespace tclib;

class CallCounter {
public:
  CallCounter() : value(0) { }
  opaque_t run();
  int value;
};

opaque_t CallCounter::run() {
  value++;
  return p2o(this);
}

TEST(thread, simple_cpp) {
  CallCounter counter;
  NativeThread thread(new_callback(&CallCounter::run, &counter));
  ASSERT_EQ(0, counter.value);
  ASSERT_TRUE(thread.start());
  opaque_t join_result = o0();
  ASSERT_TRUE(thread.join(&join_result));
  ASSERT_PTREQ(&counter, o2p(join_result));
  ASSERT_EQ(1, counter.value);
}

static opaque_t run_call_counter_bridge(opaque_t raw_data) {
  CallCounter *self = static_cast<CallCounter*>(o2p(raw_data));
  return self->run();
}

TEST(thread, simple_c) {
  CallCounter counter;
  nullary_callback_t *callback = nullary_callback_new_1(run_call_counter_bridge,
      p2o(&counter));
  native_thread_t *thread = native_thread_new(callback);
  ASSERT_EQ(0, counter.value);
  ASSERT_TRUE(native_thread_start(thread));
  opaque_t join_result = o0();
  ASSERT_TRUE(native_thread_join(thread, &join_result));
  ASSERT_PTREQ(&counter, o2p(join_result));
  ASSERT_EQ(1, counter.value);
  native_thread_destroy(thread);
  callback_destroy(callback);
}

opaque_t check_thread_not_equal(native_thread_id_t that) {
  native_thread_id_t own = NativeThread::get_current_id();
  ASSERT_TRUE(NativeThread::ids_equal(own, own));
  ASSERT_FALSE(NativeThread::ids_equal(own, that));
  return o0();
}

TEST(thread, cpp_equality) {
  native_thread_id_t current = NativeThread::get_current_id();
  ASSERT_TRUE(NativeThread::ids_equal(current, current));
  ASSERT_TRUE(NativeThread::ids_equal(NativeThread::get_current_id(),
      NativeThread::get_current_id()));
  NativeThread thread(new_callback(&check_thread_not_equal, current));
  ASSERT_TRUE(thread.start());
  ASSERT_TRUE(thread.join(NULL));
}

TEST(thread, sleep) {
  RealTimeClock *clock = RealTimeClock::system();
  uint64_t start = clock->time_since_epoch_utc().to_millis();
  NativeThread::sleep(Duration::millis(150));
  uint64_t end = clock->time_since_epoch_utc().to_millis();
  // There seems to be some difference between the clock that's sleeping and the
  // real time clock on windows so allow the duration to be smaller there.
  ASSERT_REL(end - start, >=, IF_MSVC(100, 150));
}
