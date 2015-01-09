//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "async/promise-inl.hh"

using namespace tclib;

TEST(promise, simple) {
  promise_t<int> p = promise_t<int>::empty();
  ASSERT_TRUE(p.is_empty());
  ASSERT_EQ(0, p.get_value(0));
  ASSERT_EQ(5, p.get_value(5));
  p.fulfill(10);
  ASSERT_FALSE(p.is_empty());
  ASSERT_EQ(10, p.get_value(0));
}

TEST(promise, simple_error) {
  promise_t<void*, int> p = promise_t<void*, int>::empty();
  ASSERT_TRUE(p.is_empty());
  ASSERT_EQ(0, p.get_error(0));
  ASSERT_EQ(5, p.get_error(5));
  p.fail(10);
  ASSERT_FALSE(p.is_empty());
  ASSERT_EQ(10, p.get_error(0));
}

TEST(promise, reffing) {
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

TEST(promise, destruct) {
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

TEST(promise, action_on_success) {
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

TEST(promise, action_on_failure) {
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

TEST(promise, then_success) {
  promise_t<int> a = promise_t<int>::empty();
  promise_t<int> b = a.then<int>(new_callback(shift_plus_n, 4));
  promise_t<int> c = b.then<int>(new_callback(shift_plus_n, 5));
  promise_t<int> d = c.then<int>(new_callback(shift_plus_n, 6));
  promise_t<bool> is_d_even = d.then<bool>(is_even);
  a.fulfill(8);
  ASSERT_EQ(8456, d.get_value(0));
  ASSERT_EQ(845, c.get_value(0));
  ASSERT_EQ(84, b.get_value(0));
  ASSERT_EQ(true, is_d_even.get_value(false));
}

TEST(promise, then_failure) {
  promise_t<int, int> a = promise_t<int, int>::empty();
  promise_t<int, int> b = a.then<int>(new_callback(shift_plus_n, 7));
  promise_t<int, int> c = b.then<int>(new_callback(shift_plus_n, 8));
  promise_t<int, int> d = c.then<int>(new_callback(shift_plus_n, 9));
  a.fail(100);
  ASSERT_EQ(100, d.get_error(0));
  ASSERT_EQ(100, c.get_error(0));
  ASSERT_EQ(100, b.get_error(0));
}
