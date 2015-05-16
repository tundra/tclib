//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "sync/process.hh"
#include "sync/pipe.hh"

BEGIN_C_INCLUDES
#include "utils/strbuf.h"
#include "utils/string-inl.h"
END_C_INCLUDES

using namespace tclib;

TEST(process_cpp, exec_missing) {
  NativeProcess process;
  ASSERT_TRUE(process.start("test_process_cpp_exec_fail_missing_executable", 0, NULL));
  ASSERT_TRUE(process.wait());
  ASSERT_TRUE(process.exit_code() != 0);
}

// Returns the path to the durian executable.
static const char *get_durian_main() {
  const char *result = getenv("DURIAN_MAIN");
  ASSERT_TRUE(result != NULL);
  return result;
}

// A process that records interaction.
class RecordingProcess : public NativeProcess {
public:
  RecordingProcess();
  ~RecordingProcess();

  // Waits for the process to complete, meanwhile capturing output.
  void complete();

  // Returns a string containing the text written to the process' standard
  // output.
  const char *out();

private:
  NativePipe stdout_pipe_;
  string_buffer_t stdout_buf_;
};

RecordingProcess::RecordingProcess() {
  ASSERT_TRUE(stdout_pipe_.open(NativePipe::pfInherit));
  set_stdout(stdout_pipe_.out());
  string_buffer_init(&stdout_buf_);
}

RecordingProcess::~RecordingProcess() {
  string_buffer_dispose(&stdout_buf_);
}

void RecordingProcess::complete() {
  InStream *out = stdout_pipe_.in();
  size_t last_read = 1;
  while (last_read > 0 || !out->at_eof()) {
    char chars[256];
    last_read = out->read_bytes(chars, 256);
    ASSERT_TRUE(last_read <= 256);
    string_buffer_append(&stdout_buf_, new_string(chars, last_read));
  }
  // We know the process has completed but still need to call wait for the
  // state to be updated.
  wait();
}

const char *RecordingProcess::out() {
  return string_buffer_flush(&stdout_buf_).chars;
}

TEST(process_cpp, return_value) {
  RecordingProcess process;
  const char *argv[2] = {"--exit-code", "66"};
  ASSERT_TRUE(process.start(get_durian_main(), 2, argv));
  process.complete();
  ASSERT_EQ(66, process.exit_code());
}

TEST(process_cpp, argument_passing) {
  RecordingProcess process;
  const char *argv[1] = {"foo bar baz"};
  ASSERT_TRUE(process.start(get_durian_main(), 1, argv));
  process.complete();
  ASSERT_EQ(0, process.exit_code());
}

#if defined(IS_MSVC)
#  include "c/winhdr.h"
#endif

TEST(process_cpp, msvc_sizes) {
#if defined(IS_MSVC)
  // If this fails it should be easy to fix, just bump up the size of the
  // platform process type.
  ASSERT_REL(sizeof(platform_process_t), >=, sizeof(PROCESS_INFORMATION));
#endif
}
