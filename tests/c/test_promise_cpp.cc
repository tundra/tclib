//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "async/promise-inl.hh"
#include "sync/thread.hh"
#include "test/unittest.hh"

using namespace tclib;

TEST(promise_cpp, simple) {
  promise_t<int, int> p = promise_t<int, int>::empty();
  ASSERT_FALSE(p.is_resolved());
  ASSERT_FALSE(p.has_succeeded());
  ASSERT_FALSE(p.has_failed());
  ASSERT_EQ(0, p.peek_value(0));
  ASSERT_EQ(5, p.peek_value(5));
  p.fulfill(10);
  ASSERT_TRUE(p.is_resolved());
  ASSERT_TRUE(p.has_succeeded());
  ASSERT_FALSE(p.has_failed());
  ASSERT_EQ(10, p.peek_value(0));
  ASSERT_EQ(0, p.peek_error(0));
}

TEST(promise_cpp, simple_sync) {
  sync_promise_t<int, int> p = sync_promise_t<int, int>::empty();
  ASSERT_FALSE(p.is_resolved());
  ASSERT_FALSE(p.has_succeeded());
  ASSERT_FALSE(p.has_failed());
  ASSERT_EQ(0, p.peek_value(0));
  ASSERT_EQ(5, p.peek_value(5));
  p.fulfill(10);
  ASSERT_TRUE(p.is_resolved());
  ASSERT_TRUE(p.has_succeeded());
  ASSERT_FALSE(p.has_failed());
  ASSERT_EQ(10, p.peek_value(0));
  ASSERT_EQ(0, p.peek_error(0));
}

TEST(promise_cpp, simple_error) {
  promise_t<void*, int> p = promise_t<void*, int>::empty();
  ASSERT_FALSE(p.is_resolved());
  ASSERT_EQ(0, p.peek_error(0));
  ASSERT_EQ(5, p.peek_error(5));
  p.fail(10);
  ASSERT_TRUE(p.is_resolved());
  ASSERT_TRUE(p.has_failed());
  ASSERT_FALSE(p.has_succeeded());
  ASSERT_EQ(10, p.peek_error(0));
  ASSERT_PTREQ(NULL, p.peek_value(0));
}

TEST(promise_cpp, simple_error_sync) {
  sync_promise_t<void*, int> p = sync_promise_t<void*, int>::empty();
  ASSERT_FALSE(p.is_resolved());
  ASSERT_EQ(0, p.peek_error(0));
  ASSERT_EQ(5, p.peek_error(5));
  p.fail(10);
  ASSERT_TRUE(p.is_resolved());
  ASSERT_EQ(10, p.peek_error(0));
}

TEST(promise_cpp, reffing) {
  promise_t<int> p1 = promise_t<int>::empty();
  promise_t<int> p2 = p1;
  p1 = p2;
  {
    promise_t<int> p3 = p2;
    p3 = p1;
  }
  promise_t<int> p4 = promise_t<int>::empty();
  p1 = p4;
  p2 = p4;
}

class A {
public:
  A(int* count)
    : count_(count) { (*count_)++; }
  A(const A& that)
    : count_(that.count_) { (*count_)++; }
  A &operator=(const A& that) {
    (*count_)--;
    count_ = that.count_;
    (*count_)++;
    return *this;
  }
  ~A() { (*count_)--; }
private:
  int *count_;
};

TEST(promise_cpp, destruct) {
  int count = 0;
  {
    A a(&count);
    ASSERT_EQ(1, count);
  }
  ASSERT_EQ(0, count);
  {
    A a(&count);
    ASSERT_EQ(1, count);
    promise_t<A> pa = promise_t<A>::empty();
    pa.fulfill(a);
    ASSERT_EQ(2, count);
  }
  ASSERT_EQ(0, count);
}

static void set_value(int *dest, int value) {
  *dest = value;
}

TEST(promise_cpp, action_on_success) {
  int value_out = 0;
  promise_t<int> pi = promise_t<int>::empty();
  pi.on_success(new_callback(set_value, &value_out));
  ASSERT_EQ(0, value_out);
  pi.fulfill(5);
  ASSERT_EQ(5, value_out);
  value_out = 0;
  pi.on_success(new_callback(set_value, &value_out));
  ASSERT_EQ(5, value_out);
}

TEST(promise_cpp, action_on_failure) {
  int value_out = 0;
  promise_t<void*, int> pi = promise_t<void*, int>::empty();
  pi.on_failure(new_callback(set_value, &value_out));
  ASSERT_EQ(0, value_out);
  pi.fail(5);
  ASSERT_EQ(5, value_out);
  value_out = 0;
  pi.on_failure(new_callback(set_value, &value_out));
  ASSERT_EQ(5, value_out);
}

static int shift_plus_n(int n, int value) {
  return (10 * value) + n;
}

static bool is_even(int value) {
  return (value & 1) == 0;
}

TEST(promise_cpp, then_success) {
  promise_t<int> a = promise_t<int>::empty();
  promise_t<int> b = a.then<int>(new_callback(shift_plus_n, 4));
  promise_t<int> c = b.then<int>(new_callback(shift_plus_n, 5));
  promise_t<int> d = c.then<int>(new_callback(shift_plus_n, 6));
  promise_t<bool> is_d_even = d.then<bool>(is_even);
  a.fulfill(8);
  ASSERT_EQ(8456, d.peek_value(0));
  ASSERT_TRUE(d.has_succeeded());
  ASSERT_FALSE(d.has_failed());
  ASSERT_EQ(845, c.peek_value(0));
  ASSERT_EQ(84, b.peek_value(0));
  ASSERT_EQ(true, is_d_even.peek_value(false));
}

TEST(promise_cpp, then_failure) {
  promise_t<int, int> a = promise_t<int, int>::empty();
  promise_t<int, int> b = a.then<int>(new_callback(shift_plus_n, 7));
  promise_t<int, int> c = b.then<int>(new_callback(shift_plus_n, 8));
  promise_t<int, int> d = c.then<int>(new_callback(shift_plus_n, 9));
  a.fail(100);
  ASSERT_EQ(100, d.peek_error(0));
  ASSERT_TRUE(d.has_failed());
  ASSERT_FALSE(d.has_succeeded());
  ASSERT_EQ(100, c.peek_error(0));
  ASSERT_EQ(100, b.peek_error(0));
}

static void *run_sync_fulfiller(promise_t<int> p) {
  ASSERT_TRUE(p.fulfill(10));
  return NULL;
}

static void *run_sync_waiter(NativeSemaphore *about_to_wait,
    NativeSemaphore *has_waited, sync_promise_t<int> p) {
  about_to_wait->release();
  ASSERT_TRUE(p.wait());
  has_waited->release();
  ASSERT_EQ(10, p.peek_value(0));
  return NULL;
}

TEST(promise_cpp, sync_wait) {
  sync_promise_t<int> p = sync_promise_t<int>::empty();
  NativeSemaphore about_to_wait(0);
  ASSERT_TRUE(about_to_wait.initialize());
  NativeSemaphore has_waited(0);
  ASSERT_TRUE(has_waited.initialize());
  NativeThread waiter(new_callback(run_sync_waiter, &about_to_wait, &has_waited, p));
  ASSERT_TRUE(waiter.start());
  ASSERT_TRUE(about_to_wait.acquire());
  // Wait for a short time to give the waiter time to actually wait.
  ASSERT_FALSE(has_waited.acquire(Duration::seconds(0.01)));
  NativeThread fulfiller(new_callback(run_sync_fulfiller, promise_t<int>(p)));
  ASSERT_TRUE(fulfiller.start());
  ASSERT_TRUE(has_waited.acquire());
  fulfiller.join();
  waiter.join();
}
