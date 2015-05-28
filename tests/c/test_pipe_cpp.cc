//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

#include "io/iop.hh"
#include "sync/pipe.hh"
#include "sync/semaphore.hh"
#include "sync/thread.hh"
#include "utils/string-inl.h"

using namespace tclib;

TEST(pipe_cpp, simple) {
  NativePipe pipe;
  ASSERT_TRUE(pipe.open(NativePipe::pfDefault));
  ASSERT_EQ(12, pipe.out()->printf("Hello, pipe!"));
  char buf[256];
  memset(buf, 0, 256);
  ReadIop read_iop(pipe.in(), buf, 256);
  ASSERT_FALSE(read_iop.at_eof());
  ASSERT_TRUE(read_iop.exec_sync());
  ASSERT_EQ(12, read_iop.read_size());
  ASSERT_C_STREQ("Hello, pipe!", buf);
  ASSERT_FALSE(read_iop.at_eof());
  ASSERT_TRUE(pipe.out()->close());
  read_iop.recycle();
  ASSERT_TRUE(read_iop.exec_sync());
  ASSERT_EQ(0, read_iop.read_size());
  ASSERT_TRUE(read_iop.at_eof());
}

static void *do_test_steps(NativeSemaphore *step, OutStream *a, OutStream *b) {
  step->acquire(duration_unlimited());
  ASSERT_EQ(1, a->write_bytes("1", 1));
  step->acquire(duration_unlimited());
  ASSERT_EQ(1, b->write_bytes("2", 1));
  step->acquire(duration_unlimited());
  ASSERT_EQ(1, a->write_bytes("3", 1));
  ASSERT_EQ(1, b->write_bytes("4", 1));
  step->acquire(duration_unlimited());
  ASSERT_TRUE(b->close());
  step->acquire(duration_unlimited());
  ASSERT_EQ(1, a->write_bytes("5", 1));
  step->acquire(duration_unlimited());
  ASSERT_TRUE(a->close());
  return NULL;
}

TEST(pipe_cpp, multiplex) {
  NativePipe a_pipe;
  ASSERT_TRUE(a_pipe.open(NativePipe::pfDefault));
  InStream *a = a_pipe.in();
  NativePipe b_pipe;
  ASSERT_TRUE(b_pipe.open(NativePipe::pfDefault));
  InStream *b = b_pipe.in();
  NativeSemaphore step(0);
  ASSERT_TRUE(step.initialize());
  NativeThread other(new_callback(do_test_steps, &step, a_pipe.out(), b_pipe.out()));
  ASSERT_TRUE(other.start());
  std::vector<InStream*> a_and_b;
  a_and_b.push_back(a);
  a_and_b.push_back(b);
  size_t index = 100;

  // First step: a has input.
  ASSERT_TRUE(step.release());
  char a_buf[256];
  char b_buf[256];

  ReadIop read_a(a, a_buf, 256);
  ReadIop read_b(b, b_buf, 256);
  IopGroup read_a_and_b;
  read_a_and_b.schedule(&read_a);
  read_a_and_b.schedule(&read_b);

  ASSERT_TRUE(read_a_and_b.wait_for_next(&index));
  ASSERT_EQ(index, 0);
  ASSERT_TRUE(read_a.has_succeeded());
  ASSERT_EQ(1, read_a.read_size());
  ASSERT_EQ('1', a_buf[0]);
  read_a.recycle();

  // Second step: b has input.
  index = 100;
  ASSERT_TRUE(step.release());
  ASSERT_TRUE(read_a_and_b.wait_for_next(&index));
  ASSERT_EQ(index, 1);
  ASSERT_TRUE(read_b.has_succeeded());
  ASSERT_EQ(1, read_b.read_size());
  ASSERT_EQ('2', b_buf[0]);
  read_b.recycle();

  // Third step: both a and b.
  ASSERT_TRUE(step.release());
  for (int i = 0; i < 2; i++) {
    size_t new_index = 0;
    ASSERT_TRUE(read_a_and_b.wait_for_next(&new_index));
    ASSERT_TRUE(i == 0 || new_index != index);
    index = new_index;
    ASSERT_TRUE(index <= 1);
    if (index == 0) {
      ASSERT_TRUE(read_a.has_succeeded());
      ASSERT_EQ(1, read_a.read_size());
      ASSERT_EQ('3', a_buf[0]);
      read_a.recycle();
    } else {
      ASSERT_TRUE(read_b.has_succeeded());
      ASSERT_EQ(1, read_b.read_size());
      ASSERT_EQ('4', b_buf[0]);
      read_b.recycle();
    }
  }

  // Detect b being closed.
  ASSERT_TRUE(step.release());
  ASSERT_TRUE(read_a_and_b.wait_for_next(&index));
  ASSERT_EQ(1, index);
  ASSERT_TRUE(read_b.is_complete());
  ASSERT_TRUE(read_b.has_succeeded());
  ASSERT_TRUE(read_b.at_eof());

  // Another successful write to a.
  ASSERT_TRUE(step.release());
  ASSERT_TRUE(read_a_and_b.wait_for_next(&index));
  ASSERT_EQ(index, 0);
  ASSERT_TRUE(read_a.has_succeeded());
  ASSERT_EQ(1, read_a.read_size());
  ASSERT_EQ('5', a_buf[0]);
  read_a.recycle();

  // Detect a being closed.
  ASSERT_TRUE(step.release());
  ASSERT_TRUE(read_a_and_b.wait_for_next(&index));
  ASSERT_EQ(0, index);
  ASSERT_TRUE(read_a.is_complete());
  ASSERT_TRUE(read_a.has_succeeded());
  ASSERT_TRUE(read_a.at_eof());

  other.join();
}
