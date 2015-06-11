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
  ASSERT_TRUE(read_iop.execute());
  ASSERT_EQ(12, read_iop.bytes_read());
  ASSERT_C_STREQ("Hello, pipe!", buf);
  ASSERT_FALSE(read_iop.at_eof());
  ASSERT_TRUE(pipe.out()->close());
  read_iop.recycle();
  ASSERT_TRUE(read_iop.execute());
  ASSERT_EQ(0, read_iop.bytes_read());
  ASSERT_TRUE(read_iop.at_eof());
}

static void *do_test_steps(NativeSemaphore *step, OutStream *a, OutStream *b) {
  step->acquire(Duration::unlimited());
  WriteIop write_a(a, "1", 1);
  ASSERT_TRUE(write_a.execute());
  ASSERT_EQ(1, write_a.bytes_written());
  step->acquire(Duration::unlimited());
  WriteIop write_b(b, "2", 1);
  ASSERT_TRUE(write_b.execute());
  ASSERT_EQ(1, write_b.bytes_written());
  step->acquire(Duration::unlimited());
  write_a.recycle("3", 1);
  ASSERT_TRUE(write_a.execute());
  ASSERT_EQ(1, write_a.bytes_written());
  write_b.recycle("4", 1);
  ASSERT_TRUE(write_b.execute());
  ASSERT_EQ(1, write_b.bytes_written());
  step->acquire(Duration::unlimited());
  ASSERT_TRUE(b->close());
  step->acquire(Duration::unlimited());
  write_a.recycle("5", 1);
  ASSERT_TRUE(write_a.execute());
  ASSERT_EQ(1, write_a.bytes_written());
  step->acquire(Duration::unlimited());
  ASSERT_TRUE(a->close());
  return NULL;
}

TEST(pipe_cpp, simple_multiplex) {
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
  opaque_t opaque_index = o0();

  // First step: a has input.
  ASSERT_TRUE(step.release());
  char a_buf[256];
  char b_buf[256];

  ReadIop read_a(a, a_buf, 256, u2o(0));
  ReadIop read_b(b, b_buf, 256, u2o(1));
  IopGroup read_a_and_b;
  read_a_and_b.schedule(&read_a);
  read_a_and_b.schedule(&read_b);
  ASSERT_EQ(2, read_a_and_b.pending_count());

  ASSERT_TRUE(read_a_and_b.wait_for_next(Duration::unlimited(), &opaque_index));
  ASSERT_EQ(o2u(opaque_index), 0);
  ASSERT_TRUE(read_a.has_succeeded());
  ASSERT_EQ(1, read_a.bytes_read());
  ASSERT_EQ('1', a_buf[0]);
  ASSERT_EQ(1, read_a_and_b.pending_count());
  read_a.recycle();
  ASSERT_EQ(2, read_a_and_b.pending_count());

  // Second step: b has input.
  opaque_index = o0();
  ASSERT_TRUE(step.release());
  ASSERT_TRUE(read_a_and_b.wait_for_next(Duration::unlimited(), &opaque_index));
  ASSERT_EQ(o2u(opaque_index), 1);
  ASSERT_TRUE(read_b.has_succeeded());
  ASSERT_EQ(1, read_b.bytes_read());
  ASSERT_EQ('2', b_buf[0]);
  ASSERT_EQ(1, read_a_and_b.pending_count());
  read_b.recycle();
  ASSERT_EQ(2, read_a_and_b.pending_count());

  // Third step: both a and b.
  ASSERT_TRUE(step.release());
  for (int i = 0; i < 2; i++) {
    opaque_t new_index = o0();
    ASSERT_TRUE(read_a_and_b.wait_for_next(Duration::unlimited(), &new_index));
    ASSERT_TRUE(i == 0 || o2u(new_index) != o2u(opaque_index));
    opaque_index = new_index;
    ASSERT_TRUE(o2u(opaque_index) <= 1);
    if (o2u(opaque_index) == 0) {
      ASSERT_TRUE(read_a.has_succeeded());
      ASSERT_EQ(1, read_a.bytes_read());
      ASSERT_EQ('3', a_buf[0]);
      ASSERT_EQ(1, read_a_and_b.pending_count());
      read_a.recycle();
      ASSERT_EQ(2, read_a_and_b.pending_count());
    } else {
      ASSERT_TRUE(read_b.has_succeeded());
      ASSERT_EQ(1, read_b.bytes_read());
      ASSERT_EQ('4', b_buf[0]);
      ASSERT_EQ(1, read_a_and_b.pending_count());
      read_b.recycle();
      ASSERT_EQ(2, read_a_and_b.pending_count());
    }
  }

  // Detect b being closed.
  ASSERT_TRUE(step.release());
  ASSERT_TRUE(read_a_and_b.wait_for_next(Duration::unlimited(), &opaque_index));
  ASSERT_EQ(1, o2u(opaque_index));
  ASSERT_TRUE(read_b.is_complete());
  ASSERT_TRUE(read_b.has_succeeded());
  ASSERT_TRUE(read_b.at_eof());
  ASSERT_EQ(1, read_a_and_b.pending_count());

  // Another successful write to a.
  ASSERT_TRUE(step.release());
  ASSERT_TRUE(read_a_and_b.wait_for_next(Duration::unlimited(), &opaque_index));
  ASSERT_EQ(o2u(opaque_index), 0);
  ASSERT_TRUE(read_a.has_succeeded());
  ASSERT_EQ(1, read_a.bytes_read());
  ASSERT_EQ('5', a_buf[0]);
  ASSERT_EQ(0, read_a_and_b.pending_count());
  read_a.recycle();
  ASSERT_EQ(1, read_a_and_b.pending_count());

  // Detect a being closed.
  ASSERT_TRUE(step.release());
  ASSERT_TRUE(read_a_and_b.wait_for_next(Duration::unlimited(), &opaque_index));
  ASSERT_EQ(0, o2u(opaque_index));
  ASSERT_TRUE(read_a.is_complete());
  ASSERT_TRUE(read_a.has_succeeded());
  ASSERT_TRUE(read_a.at_eof());
  ASSERT_EQ(0, read_a_and_b.pending_count());

  other.join();
}

#define kPipeCount 8
#define kValueCount 16384

struct Atom {
  int64_t stream;
  int64_t value;
};

static void *sync_write_streams(NativePipe *pipes) {
  for (int iv = 0; iv < kValueCount; iv++) {
    for (int is = 0; is < kPipeCount; is++) {
      Atom atom = {is, iv};
      WriteIop iop(pipes[is].out(), &atom, sizeof(Atom), o0());
      ASSERT_TRUE(iop.execute());
      ASSERT_EQ(sizeof(Atom), iop.bytes_written());
    }
  }
  for (size_t is = 0; is < kPipeCount; is++)
    ASSERT_TRUE(pipes[is].out()->close());
  return NULL;
}

static void group_read_streams(NativePipe *pipes) {
  Atom atoms[kPipeCount];
  ReadIop *iops[kPipeCount];
  size_t next_value[kPipeCount];
  IopGroup group;
  for (size_t is = 0; is < kPipeCount; is++) {
    next_value[is] = 0;
    iops[is] = new ReadIop(pipes[is].in(), &atoms[is], sizeof(Atom), u2o(is));
    group.schedule(iops[is]);
  }
  int read_count = 0;
  while (group.has_pending()) {
    opaque_t opaque_index = o0();
    ASSERT_TRUE(group.wait_for_next(Duration::unlimited(), &opaque_index));
    uint64_t index = o2u(opaque_index);
    ReadIop *iop = iops[index];
    if (iop->at_eof()) {
      ASSERT_EQ(kValueCount, next_value[index]);
    } else {
      read_count++;
      ASSERT_EQ(sizeof(Atom), iop->bytes_read());
      Atom *atom = &atoms[index];
      ASSERT_EQ(index, atom->stream);
      ASSERT_EQ(next_value[index], atom->value);
      next_value[index]++;
      atom->stream = -1;
      atom->value = 0;
      iop->recycle();
    }
  }
  ASSERT_EQ(kPipeCount * kValueCount, read_count);
  for (size_t is = 0; is < kPipeCount; is++)
    delete iops[is];
}

TEST(pipe_cpp, sync_write_group_read) {
  NativePipe pipes[kPipeCount];
  for (size_t i = 0; i < kPipeCount; i++)
    ASSERT_TRUE(pipes[i].open(NativePipe::pfDefault));
  tclib::NativeThread thread(new_callback(sync_write_streams, pipes));
  ASSERT_TRUE(thread.start());
  group_read_streams(pipes);
  ASSERT_PTREQ(NULL, thread.join());
}

static void *async_read_streams(NativePipe *pipes, size_t own_index,
    size_t *read_count_out) {
  Atom atoms[kPipeCount];
  ReadIop *iops[kPipeCount];
  IopGroup group;
  int last_value[kPipeCount];
  for (size_t is = 0; is < kPipeCount; is++) {
    iops[is] = new ReadIop(pipes[is].in(), &atoms[is], sizeof(Atom), u2o(is));
    group.schedule(iops[is]);
    last_value[is] = -1;
  }
  int read_count = 0;
  while (group.has_pending()) {
    opaque_t opaque_index = o0();
    ASSERT_TRUE(group.wait_for_next(Duration::unlimited(), &opaque_index));
    uint64_t index = o2u(opaque_index);
    NativeThread::yield(); // Just so no one thread hogs the input.
    ReadIop *iop = iops[index];
    if (!iop->at_eof()) {
      ASSERT_EQ(sizeof(Atom), iop->bytes_read());
      Atom *atom = &atoms[index];
      ASSERT_EQ(index, atom->stream);
      ASSERT_REL(atom->value, >, last_value[index]);
      last_value[index] = static_cast<int>(atom->value);
      atom->stream = -1;
      atom->value = 0;
      iop->recycle();
      read_count++;
    }
  }
  for (size_t is = 0; is < kPipeCount; is++)
    delete iops[is];
  *read_count_out = read_count;
  return NULL;
}

TEST(pipe_cpp, sync_write_contend_read) {
  NativePipe pipes[kPipeCount];
  NativeThread readers[kPipeCount];
  size_t read_counts[kPipeCount];
  for (size_t i = 0; i < kPipeCount; i++)
    ASSERT_TRUE(pipes[i].open(NativePipe::pfDefault));
  for (size_t i = 0; i < kPipeCount; i++) {
    read_counts[i] = 0;
    readers[i].set_callback(new_callback(async_read_streams, pipes, i,
        &read_counts[i]));
    ASSERT_TRUE(readers[i].start());
  }
  NativeThread thread(new_callback(sync_write_streams, pipes));
  ASSERT_TRUE(thread.start());
  for (size_t i = 0; i < kPipeCount; i++)
    ASSERT_PTREQ(NULL, readers[i].join());
  ASSERT_PTREQ(NULL, thread.join());

  size_t total_read_count = 0;
  for (size_t i = 0; i < kPipeCount; i++)
    total_read_count += read_counts[i];
  ASSERT_EQ(kPipeCount * kValueCount, total_read_count);
}

static void *run_sibling(OutStream *out, InStream *in) {
  size_t limit = 8192;
  size_t next_value_in = 0;
  size_t value_in = 0;
  size_t next_value_out = 0;
  ReadIop read_iop(in, &value_in, sizeof(value_in), u2o(0));
  WriteIop write_iop(out, &next_value_out, sizeof(next_value_out), u2o(1));
  IopGroup group;
  group.schedule(&read_iop);
  group.schedule(&write_iop);
  int live_streams = 2;
  while (live_streams > 0) {
    opaque_t opaque_index = o0();
    ASSERT_TRUE(group.wait_for_next(Duration::unlimited(), &opaque_index));
    uint64_t index = o2u(opaque_index);
    if (index == 0) {
      // Read succeeded.
      if (next_value_in == (limit + 1)) {
        ASSERT_TRUE(read_iop.at_eof());
        live_streams--;
      } else {
        ASSERT_EQ(next_value_in, value_in);
        next_value_in++;
        read_iop.recycle();
      }
    } else {
      // Write succeeded.
      ASSERT_EQ(1, index);
      if (next_value_out == limit) {
        ASSERT_TRUE(out->close());
        live_streams--;
      } else {
        next_value_out++;
        write_iop.recycle();
      }
    }
  }
  ASSERT_EQ(limit + 1, next_value_in);
  ASSERT_EQ(limit, next_value_out);
  return NULL;
}

TEST(pipe_cpp, sync_twins) {
  NativePipe red, blue;
  ASSERT_TRUE(red.open(NativePipe::pfDefault));
  ASSERT_TRUE(blue.open(NativePipe::pfDefault));
  NativeThread other(new_callback(run_sibling, red.out(), blue.in()));
  ASSERT_TRUE(other.start());
  run_sibling(blue.out(), red.in());
  ASSERT_PTREQ(NULL, other.join());
}
