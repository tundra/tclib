//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "sync/thread.hh"

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
  ASSERT_EQ(0, counter.value);
  NativeThread thread(callback_t<void*(void)>(&CallCounter::run, &counter));
  ASSERT_TRUE(thread.start());
  ASSERT_PTREQ(&counter, thread.join());
  ASSERT_EQ(1, counter.value);
}
