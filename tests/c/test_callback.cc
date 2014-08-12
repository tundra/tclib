//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "callback.hh"

using namespace tclib;

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
  callback_t<int(void)> cb0 = add_global_tick;
  ASSERT_EQ(0, global_ticks);
  ASSERT_EQ(9, cb0());
  ASSERT_EQ(1, global_ticks);
  int local_ticks = 0;
  callback_t<int(int*)> cb1 = add_local_tick;
  ASSERT_EQ(0, local_ticks);
  ASSERT_EQ(19, cb1(&local_ticks));
  ASSERT_EQ(1, local_ticks);
  callback_t<int(void)> cb2(add_local_tick, &local_ticks);
  ASSERT_EQ(1, local_ticks);
  ASSERT_EQ(19, cb2());
  ASSERT_EQ(2, local_ticks);
  callback_t<int(int*, int)> cb3 = add_n_to_local_tick;
  ASSERT_EQ(2, local_ticks);
  ASSERT_EQ(29, cb3(&local_ticks, 4));
  ASSERT_EQ(6, local_ticks);
  callback_t<int(int)> cb4(add_n_to_local_tick, &local_ticks);
  ASSERT_EQ(6, local_ticks);
  ASSERT_EQ(29, cb4(5));
  ASSERT_EQ(11, local_ticks);
  callback_t<int(void)> cb5(add_n_to_local_tick, &local_ticks, 7);
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
  callback_t<int(Testy::*)(void)> c0 = &Testy::add_one;
  Testy testy;
  ASSERT_EQ(0, testy.ticks_);
  ASSERT_EQ(39, c0(&testy));
  ASSERT_EQ(1, testy.ticks_);
  callback_t<int(void)> c1(&Testy::add_one, &testy);
  ASSERT_EQ(1, testy.ticks_);
  ASSERT_EQ(39, c1());
  ASSERT_EQ(2, testy.ticks_);
  callback_t<int(Testy::*)(int)> c2 = &Testy::add_n;
  ASSERT_EQ(2, testy.ticks_);
  ASSERT_EQ(49, c2(&testy, 3));
  ASSERT_EQ(5, testy.ticks_);
  callback_t<int(int)> c3(&Testy::add_n, &testy);
  ASSERT_EQ(5, testy.ticks_);
  ASSERT_EQ(49, c3(5));
  ASSERT_EQ(10, testy.ticks_);
  callback_t<int(void)> c4(&Testy::add_n, &testy, 7);
  ASSERT_EQ(10, testy.ticks_);
  ASSERT_EQ(49, c4());
  ASSERT_EQ(17, testy.ticks_);
}
