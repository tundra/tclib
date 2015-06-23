//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "sync/pipe.h"
#include "sync/process.h"
#include "utils/callback.h"
#include "utils/strbuf.h"
#include "utils/string-inl.h"
END_C_INCLUDES

using namespace tclib;

// Returns the path to the durian executable.
static const char *get_durian_main() {
  const char *result = getenv("DURIAN_MAIN");
  ASSERT_TRUE(result != NULL);
  return result;
}

TEST(process_c, return_value) {
  // Create a subprocess with out and err redirected.
  native_process_t process;
  ASSERT_TRUE(native_process_initialize(&process));
  native_pipe_t stdout_pipe;
  ASSERT_TRUE(native_pipe_open(&stdout_pipe));
  stream_redirect_t *stdout_redir = stream_redirect_from_pipe(&stdout_pipe, pdOut);
  native_process_set_stdout(&process, stdout_redir);
  native_pipe_t stderr_pipe;
  ASSERT_TRUE(native_pipe_open(&stderr_pipe));
  stream_redirect_t *stderr_redir = stream_redirect_from_pipe(&stderr_pipe, pdOut);
  native_process_set_stderr(&process, stderr_redir);

  // Launch the process.
  const char *argv[2] = {"--exit-code", "77"};
  ASSERT_TRUE(native_process_start(&process, get_durian_main(), 2, argv));

  // Consume all the output, otherwise the process may block waiting to be able
  // to write.
  iop_group_t group;
  iop_group_initialize(&group);
  char stdout_buf[256];
  read_iop_t read_stdout;
  read_iop_init(&read_stdout, native_pipe_in(&stdout_pipe), stdout_buf, 256, o0());
  iop_group_schedule_read(&group, &read_stdout);
  char stderr_buf[256];
  read_iop_t read_stderr;
  read_iop_init(&read_stderr, native_pipe_in(&stderr_pipe), stderr_buf, 256, o0());
  iop_group_schedule_read(&group, &read_stderr);
  while (iop_group_pending_count(&group) > 0) {
    iop_t *iop = NULL;
    ASSERT_TRUE(iop_group_wait_for_next(&group, duration_unlimited(), &iop));
    if (!read_iop_at_eof((read_iop_t*) iop))
      iop_recycle_same_state(iop);
  }
  read_iop_dispose(&read_stderr);
  read_iop_dispose(&read_stdout);
  iop_group_dispose(&group);

  // Join with the process and check that it ran as expected.
  ASSERT_TRUE(native_process_wait(&process));
  ASSERT_EQ(77, native_process_exit_code(&process));

  // Clean up.
  stream_redirect_destroy(stderr_redir);
  stream_redirect_destroy(stdout_redir);
  native_pipe_dispose(&stdout_pipe);
  native_pipe_dispose(&stderr_pipe);
  native_process_dispose(&process);
}
