//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "utils/string-inl.h"

BEGIN_C_INCLUDES
#include "io/iop.h"
#include "sync/pipe.h"
#include "utils/callback.h"
#include "sync/semaphore.h"
#include "sync/thread.h"
END_C_INCLUDES

TEST(pipe_c, simple) {
  native_pipe_t pipe;
  ASSERT_TRUE(native_pipe_open(&pipe));
  out_stream_t *out = native_pipe_out(&pipe);
  in_stream_t *in = native_pipe_in(&pipe);
  ASSERT_EQ(12, out_stream_printf(out, "Hello, pipe!"));
  char buf[256];
  memset(buf, 0, 256);
  read_iop_t read_iop;
  read_iop_init(&read_iop, in, buf, 256, u2o(63));
  ASSERT_FALSE(read_iop_at_eof(&read_iop));
  ASSERT_EQ(63, o2u(read_iop_extra(&read_iop)));
  ASSERT_TRUE(read_iop_execute(&read_iop));
  ASSERT_EQ(12, read_iop_bytes_read(&read_iop));
  ASSERT_C_STREQ("Hello, pipe!", buf);
  ASSERT_FALSE(read_iop_at_eof(&read_iop));
  ASSERT_TRUE(out_stream_close(out));
  read_iop_recycle_same_state(&read_iop);
  ASSERT_TRUE(read_iop_execute(&read_iop));
  ASSERT_EQ(0, read_iop_bytes_read(&read_iop));
  ASSERT_TRUE(read_iop_at_eof(&read_iop));
  read_iop_dispose(&read_iop);
  native_pipe_dispose(&pipe);
}

static opaque_t do_test_steps(opaque_t opaque_step, opaque_t opaque_a,
    opaque_t opaque_b) {
  native_semaphore_t *step = (native_semaphore_t*) o2p(opaque_step);
  out_stream_t *a = (out_stream_t*) o2p(opaque_a);
  out_stream_t *b = (out_stream_t*) o2p(opaque_b);

  ASSERT_TRUE(native_semaphore_acquire(step, duration_unlimited()));
  write_iop_t write_a;
  write_iop_init(&write_a, a, "1", 1, o0());
  ASSERT_TRUE(write_iop_execute(&write_a));
  ASSERT_EQ(1, write_iop_bytes_written(&write_a));

  ASSERT_TRUE(native_semaphore_acquire(step, duration_unlimited()));
  write_iop_t write_b;
  write_iop_init(&write_b, b, "2", 1, o0());
  ASSERT_TRUE(write_iop_execute(&write_b));
  ASSERT_EQ(1, write_iop_bytes_written(&write_b));

  ASSERT_TRUE(native_semaphore_acquire(step, duration_unlimited()));
  ASSERT_TRUE(out_stream_close(b));

  ASSERT_TRUE(native_semaphore_acquire(step, duration_unlimited()));
  write_iop_init(&write_a, a, "3", 1, o0());
  ASSERT_TRUE(write_iop_execute(&write_a));
  ASSERT_EQ(1, write_iop_bytes_written(&write_a));

  ASSERT_TRUE(native_semaphore_acquire(step, duration_unlimited()));
  ASSERT_TRUE(out_stream_close(a));

  return o0();
}

TEST(pipe_c, simple_multiplex) {
  native_pipe_t a_pipe;
  ASSERT_TRUE(native_pipe_open(&a_pipe));
  in_stream_t *a = native_pipe_in(&a_pipe);
  native_pipe_t b_pipe;
  ASSERT_TRUE(native_pipe_open(&b_pipe));
  in_stream_t *b = native_pipe_in(&b_pipe);
  native_semaphore_t step;
  native_semaphore_construct_with_count(&step, 0);
  ASSERT_TRUE(native_semaphore_initialize(&step));
  nullary_callback_t *do_test_steps_callback = nullary_callback_new_3(do_test_steps,
      p2o(&step), p2o(native_pipe_out(&a_pipe)), p2o(native_pipe_out(&b_pipe)));
  native_thread_t *other = native_thread_new(do_test_steps_callback);
  ASSERT_TRUE(native_thread_start(other));
  iop_t *next = NULL;

  ASSERT_TRUE(native_semaphore_release(&step));
  char a_buf[256];
  char b_buf[256];

  read_iop_t read_a;
  read_iop_init(&read_a, a, a_buf, 256, u2o(0));
  read_iop_t read_b;
  read_iop_init(&read_b, b, b_buf, 256, u2o(1));
  iop_group_t read_a_and_b;
  iop_group_initialize(&read_a_and_b);
  iop_group_schedule_read(&read_a_and_b, &read_a);
  iop_group_schedule_read(&read_a_and_b, &read_b);
  ASSERT_EQ(2, iop_group_pending_count(&read_a_and_b));

  ASSERT_TRUE(iop_group_wait_for_next(&read_a_and_b, duration_unlimited(),
      &next));
  ASSERT_EQ(o2u(iop_extra(next)), 0);
  ASSERT_TRUE(read_iop_has_succeeded(&read_a));
  ASSERT_EQ(1, read_iop_bytes_read(&read_a));
  ASSERT_EQ('1', a_buf[0]);
  ASSERT_EQ(1, iop_group_pending_count(&read_a_and_b));
  read_iop_recycle_same_state(&read_a);
  ASSERT_EQ(2, iop_group_pending_count(&read_a_and_b));

  next = NULL;
  ASSERT_TRUE(native_semaphore_release(&step));
  ASSERT_TRUE(iop_group_wait_for_next(&read_a_and_b, duration_unlimited(),
      &next));
  ASSERT_EQ(o2u(iop_extra(next)), 1);
  ASSERT_TRUE(read_iop_has_succeeded(&read_b));
  ASSERT_EQ(1, read_iop_bytes_read(&read_b));
  ASSERT_EQ('2', b_buf[0]);
  ASSERT_EQ(1, iop_group_pending_count(&read_a_and_b));
  read_iop_recycle_same_state(&read_b);
  ASSERT_EQ(2, iop_group_pending_count(&read_a_and_b));

  ASSERT_TRUE(native_semaphore_release(&step));
  ASSERT_TRUE(iop_group_wait_for_next(&read_a_and_b, duration_unlimited(),
      &next));
  ASSERT_EQ(o2u(iop_extra(next)), 1);
  ASSERT_TRUE(read_iop_has_succeeded(&read_b));
  ASSERT_EQ(0, read_iop_bytes_read(&read_b));
  ASSERT_TRUE(read_iop_at_eof(&read_b));
  ASSERT_EQ(1, iop_group_pending_count(&read_a_and_b));

  ASSERT_TRUE(native_semaphore_release(&step));
  ASSERT_TRUE(iop_group_wait_for_next(&read_a_and_b, duration_unlimited(),
      &next));
  ASSERT_EQ(o2u(iop_extra(next)), 0);
  ASSERT_TRUE(read_iop_has_succeeded(&read_a));
  ASSERT_EQ(1, read_iop_bytes_read(&read_a));
  ASSERT_EQ('3', a_buf[0]);
  ASSERT_EQ(0, iop_group_pending_count(&read_a_and_b));
  read_iop_recycle_same_state(&read_a);
  ASSERT_EQ(1, iop_group_pending_count(&read_a_and_b));

  ASSERT_TRUE(native_semaphore_release(&step));
  ASSERT_TRUE(iop_group_wait_for_next(&read_a_and_b, duration_unlimited(),
      &next));
  ASSERT_EQ(o2u(iop_extra(next)), 0);
  ASSERT_TRUE(read_iop_has_succeeded(&read_a));
  ASSERT_EQ(0, read_iop_bytes_read(&read_a));
  ASSERT_TRUE(read_iop_at_eof(&read_a));
  ASSERT_EQ(0, iop_group_pending_count(&read_a_and_b));

  iop_group_dispose(&read_a_and_b);
  opaque_t result = u2o(1);
  ASSERT_TRUE(native_thread_join(other, &result));
  ASSERT_PTREQ(NULL, o2p(result));
  native_thread_destroy(other);
  callback_destroy(do_test_steps_callback);
  native_semaphore_dispose(&step);
  native_pipe_dispose(&a_pipe);
  native_pipe_dispose(&b_pipe);
}
