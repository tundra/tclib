//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "callback.hh"

using namespace tclib;

class A { };

TEST(callback, sizes) {
  typedef void (A::*void_method_t)(void);
  ASSERT_TRUE(sizeof(void_method_t) <= (opaque_invoker_t::kMaxSize));
  typedef A (A::*return_class_method_t)(void);
  ASSERT_TRUE(sizeof(return_class_method_t) <= (opaque_invoker_t::kMaxSize));
  typedef A (A::*accept_class_method_t)(A, A, A);
  ASSERT_TRUE(sizeof(accept_class_method_t) <= (opaque_invoker_t::kMaxSize));
  typedef void (*void_function_t)(void);
  ASSERT_TRUE(sizeof(void_function_t) <= (opaque_invoker_t::kMaxSize));
  typedef A (*return_class_function_t)(void);
  ASSERT_TRUE(sizeof(return_class_function_t) <= (opaque_invoker_t::kMaxSize));
  typedef A (*accept_class_function_t)(A, A, A);
  ASSERT_TRUE(sizeof(accept_class_function_t) <= (opaque_invoker_t::kMaxSize));
}

static int global_ticks;

int add_global_tick() {
  global_ticks++;
  return 9;
}

int add_local_tick(int *ticks) {
  (*ticks)++;
  return 19;
}

int add_n_to_local_tick(int *ticks, int n) {
  (*ticks) += n;
  return 29;
}

TEST(callback, functions) {
  global_ticks = 0;
  callback_t<int(void)> cb0 = new_callback(add_global_tick);
  ASSERT_FALSE(cb0.is_empty());
  callback_t<int(void)> cb0a = add_global_tick;
  ASSERT_FALSE(cb0a.is_empty());
  callback_t<int(void)> eb0e = empty_callback();
  ASSERT_TRUE(eb0e.is_empty());
  ASSERT_EQ(0, global_ticks);
  ASSERT_EQ(9, cb0());
  ASSERT_EQ(1, global_ticks);
  int local_ticks = 0;
  callback_t<int(int*)> cb1 = new_callback(&add_local_tick);
  callback_t<int(int*)> cb1a = add_local_tick;
  callback_t<int(int*)> cb1e = empty_callback();
  ASSERT_TRUE(cb1e.is_empty());
  ASSERT_EQ(0, local_ticks);
  ASSERT_EQ(19, cb1(&local_ticks));
  ASSERT_EQ(1, local_ticks);
  callback_t<int(void)> cb2 = new_callback(&add_local_tick, &local_ticks);
  ASSERT_EQ(1, local_ticks);
  ASSERT_EQ(19, cb2());
  ASSERT_EQ(2, local_ticks);
  callback_t<int(int*, int)> cb3 = new_callback(&add_n_to_local_tick);
  callback_t<int(int*, int)> cb3a = add_n_to_local_tick;
  callback_t<int(int*, int)> cb3e = empty_callback();
  ASSERT_EQ(2, local_ticks);
  ASSERT_EQ(29, cb3(&local_ticks, 4));
  ASSERT_EQ(6, local_ticks);
  callback_t<int(int)> cb4 = new_callback(&add_n_to_local_tick, &local_ticks);
  ASSERT_EQ(6, local_ticks);
  ASSERT_EQ(29, cb4(5));
  ASSERT_EQ(11, local_ticks);
  callback_t<int(void)> cb5 = new_callback(&add_n_to_local_tick, &local_ticks, 7);
  ASSERT_EQ(11, local_ticks);
  ASSERT_EQ(29, cb5());
  ASSERT_EQ(18, local_ticks);
  {
    callback_t<int(void)> cb6 = cb5;
    ASSERT_EQ(18, local_ticks);
    ASSERT_EQ(29, cb6());
    ASSERT_EQ(25, local_ticks);
  }
  ASSERT_EQ(25, local_ticks);
  ASSERT_EQ(29, cb5());
  ASSERT_EQ(32, local_ticks);
  {
    callback_t<int(void)> cb7;
    cb7 = cb5;
    ASSERT_EQ(32, local_ticks);
    ASSERT_EQ(29, cb7());
    ASSERT_EQ(39, local_ticks);
  }
  ASSERT_EQ(39, local_ticks);
  ASSERT_EQ(29, cb5());
  ASSERT_EQ(46, local_ticks);
}

class Testy {
public:
  Testy() : ticks_(0) { }
  int add_one();
  int add_n(int n);
  int ticks_;
};

int Testy::add_one() {
  ticks_++;
  return 39;
}

int Testy::add_n(int n) {
  ticks_ += n;
  return 49;
}

TEST(callback, methods) {
  callback_t<int(Testy*)> c0 = new_callback(&Testy::add_one);
  callback_t<int(Testy*)> c0e = empty_callback();
  ASSERT_TRUE(c0e.is_empty());
  Testy testy;
  ASSERT_EQ(0, testy.ticks_);
  ASSERT_EQ(39, c0(&testy));
  ASSERT_EQ(1, testy.ticks_);
  callback_t<int(void)> c1 = new_callback(&Testy::add_one, &testy);
  ASSERT_EQ(1, testy.ticks_);
  ASSERT_EQ(39, c1());
  ASSERT_EQ(2, testy.ticks_);
  callback_t<int(Testy*, int)> c2 = new_callback(&Testy::add_n);
  ASSERT_EQ(2, testy.ticks_);
  ASSERT_EQ(49, c2(&testy, 3));
  ASSERT_EQ(5, testy.ticks_);
  callback_t<int(int)> c3 = new_callback(&Testy::add_n, &testy);
  ASSERT_EQ(5, testy.ticks_);
  ASSERT_EQ(49, c3(5));
  ASSERT_EQ(10, testy.ticks_);
  callback_t<int(void)> c4 = new_callback(&Testy::add_n, &testy, 7);
  ASSERT_EQ(10, testy.ticks_);
  ASSERT_EQ(49, c4());
  ASSERT_EQ(17, testy.ticks_);
}

TEST(callback, empty) {
  callback_t<int(void*)> e = empty_callback();
  ASSERT_TRUE(e.is_empty());
}
