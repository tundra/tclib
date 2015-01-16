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

int return_one() {
  return 1;
}

TEST(callback, link_one) {
  callback_t<int(void)> callback = return_one;
  ASSERT_EQ(1, callback());
}

int f0() { return 100; }
int f1(int a) { return a; }
int f2(int a, int b) { return 10 * f1(a) + b; }
int f3(int a, int b, int c) { return 10 * f2(a, b) + c; }
int f4(int a, int b, int c, int d) { return 10 * f3(a, b, c) + d; }
int f5(int a, int b, int c, int d, int e) { return 10 * f4(a, b, c, d) + e; }
int f6(int a, int b, int c, int d, int e, int f) { return 10 * f5(a, b, c, d, e) + f; }

class M {
public:
  int m0() { return 200; }
  int m1(int a) { return a; }
  int m2(int a, int b) { return m1(a + 10 * b); }
  int m3(int a, int b, int c) { return m2(a, b + 10 * c); }
  int m4(int a, int b, int c, int d) { return m3(a, b, c + 10 * d); }
  int m5(int a, int b, int c, int d, int e) { return m4(a, b, c, d + 10 * e); }
};

TEST(callback, matrix) {
  // Test the product of all bound and unbound argument counts. We support up to
  // 3 unbound arguments and up to 2 bound ones, with functions and methods.
  // That means a total of 23 different callback constructors which should all
  // appear exactly once here (that is, 4 unbound times 3 bound time 2 types,
  // minus 0-unbound/0-bound for methods because they need at least one
  // argument).

  M m;

  // Unbound: 0
  callback_t<int(void)> if0 = new_callback(f0);
  ASSERT_EQ(100, if0());
  callback_t<int(void)> if1 = new_callback(f1, 1);
  ASSERT_EQ(1, if1());
  callback_t<int(void)> if2 = new_callback(f2, 2, 3);
  ASSERT_EQ(23, if2());
  callback_t<int(void)> if3 = new_callback(f3, 4, 5, 6);
  ASSERT_EQ(456, if3());
  callback_t<int(void)> im0 = new_callback(&M::m0, &m);
  ASSERT_EQ(200, im0());
  callback_t<int(void)> im1 = new_callback(&M::m1, &m, 1);
  ASSERT_EQ(1, im1());
  callback_t<int(void)> im2 = new_callback(&M::m2, &m, 2, 3);
  ASSERT_EQ(32, im2());

  // Unbound: 1
  callback_t<int(int)> iif1 = new_callback(f1);
  ASSERT_EQ(4, iif1(4));
  callback_t<int(int)> iif2 = new_callback(f2, 5);
  ASSERT_EQ(56, iif2(6));
  callback_t<int(int)> iif3 = new_callback(f3, 7, 8);
  ASSERT_EQ(789, iif3(9));
  callback_t<int(int)> iif4 = new_callback(f4, 10, 11, 12);
  ASSERT_EQ(11233, iif4(13));
  callback_t<int(M*)> imm0 = new_callback(&M::m0);
  ASSERT_EQ(200, imm0(&m));
  callback_t<int(int)> iim1 = new_callback(&M::m1, &m);
  ASSERT_EQ(4, iim1(4));
  callback_t<int(int)> iim2 = new_callback(&M::m2, &m, 5);
  ASSERT_EQ(65, iim2(6));
  callback_t<int(int)> iim3 = new_callback(&M::m3, &m, 7, 8);
  ASSERT_EQ(95, iim2(9));

  // Unbound: 2
  callback_t<int(int, int)> iiif2 = new_callback(f2);
  ASSERT_EQ(78, iiif2(7, 8));
  callback_t<int(int, int)> iiif3 = new_callback(f3, 9);
  ASSERT_EQ(1011, iiif3(10, 11));
  callback_t<int(int, int)> iiif4 = new_callback(f4, 12, 13);
  ASSERT_EQ(13455, iiif4(14, 15));
  callback_t<int(int, int)> iiif5 = new_callback(f5, 16, 17, 18);
  ASSERT_EQ(179010, iiif5(19, 20));
  callback_t<int(M*, int)> imim1 = new_callback(&M::m1);
  ASSERT_EQ(9, imim1(&m, 9));
  callback_t<int(int, int)> iiim2 = new_callback(&M::m2, &m);
  ASSERT_EQ(120, iiim2(10, 11));
  callback_t<int(int, int)> iiim3 = new_callback(&M::m3, &m, 12);
  ASSERT_EQ(1542, iiim3(13, 14));
  callback_t<int(int, int)> iiim4 = new_callback(&M::m4, &m, 15, 16);
  ASSERT_EQ(19875, iiim4(17, 18));

  // Unbound: 3
  callback_t<int(int, int, int)> iiiif3 = new_callback(f3);
  ASSERT_EQ(1344, iiiif3(12, 13, 14));
  callback_t<int(int, int, int)> iiiif4 = new_callback(f4, 15);
  ASSERT_EQ(16788, iiiif4(16, 17, 18));
  callback_t<int(int, int, int)> iiiif5 = new_callback(f5, 19, 20);
  ASSERT_EQ(212343, iiiif5(21, 22, 23));
  callback_t<int(int, int, int)> iiiif6 = new_callback(f6, 24, 25, 26);
  ASSERT_EQ(2679009, iiiif6(27, 28, 29));
  callback_t<int(M*, int, int)> iiiim2 = new_callback(&M::m2);
  ASSERT_EQ(175, iiiim2(&m, 15, 16));
  callback_t<int(int, int, int)> iiiim3 = new_callback(&M::m3, &m);
  ASSERT_EQ(2097, iiiim3(17, 18, 19));
  callback_t<int(int, int, int)> iiiim4 = new_callback(&M::m4, &m, 20);
  ASSERT_EQ(25430, iiiim4(21, 22, 23));
  callback_t<int(int, int, int)> iiiim5 = new_callback(&M::m5, &m, 24, 25);
  ASSERT_EQ(309874, iiiim5(26, 27, 28));
}
