//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "sync/process.hh"
#include "sync/pipe.hh"
#include "utils/callback.hh"

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

  // Extracts the recorded argc from the process' output, if no argc can be
  // found returns -1.
  int read_argc();

  // Returns the value of the index'th entry in the process' argv. The given
  // char buffer is scratch memory to use to store the result. If no argument
  // is found returns NULL.
  const char *read_argv(int index, char *scratch);

public:
  // Iterate through the process' standard output and, for each line, invoke the
  // given callback. The first time the scan is successful (returns true) this
  // function returns.
  bool for_each_stdout_line(callback_t<bool(const char *line)> callback);

  NativePipe stdout_pipe_;
  string_buffer_t stdout_buf_;
  const char *stdout_str_;
};

RecordingProcess::RecordingProcess()
  : stdout_str_(NULL) {
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
  stdout_str_ = string_buffer_flush(&stdout_buf_).chars;
}

static bool scan_argc(int *argc, const char *line) {
  return sscanf(line, "ARGC: {%i}", argc) > 0;
}

int RecordingProcess::read_argc() {
  int result = -1;
  for_each_stdout_line(new_callback(scan_argc, &result));
  return result;
}

static bool scan_argv(int index, char *out, const char *line) {
  char fmt[256];
  sprintf(fmt, "ARGV[%i]: {%%[^}]}", index);
  return sscanf(line, fmt, out) > 0;
}

const char *RecordingProcess::read_argv(int index, char *out) {
  return for_each_stdout_line(new_callback(scan_argv, index, out))
    ? out
    : NULL;
}

bool RecordingProcess::for_each_stdout_line(callback_t<bool(const char *line)> callback) {
  const char *output = stdout_str_;
  for (size_t i = 0; output[i] != '\0'; i++) {
    const char *current;
    if (i == 0) {
      current = output;
    } else if (output[i] == '\n') {
      current = &output[i + 1];
    } else {
      current = NULL;
    }
    if (current != NULL && callback(current))
      return true;
  }
  return false;
}

TEST(process_cpp, return_value) {
  RecordingProcess process;
  const char *argv[2] = {"--exit-code", "66"};
  ASSERT_TRUE(process.start(get_durian_main(), 2, argv));
  process.complete();
  ASSERT_EQ(66, process.exit_code());
  ASSERT_EQ(3, process.read_argc());
  char argbuf[1024];
  ASSERT_C_STREQ(get_durian_main(), process.read_argv(0, argbuf));
  ASSERT_C_STREQ("--exit-code", process.read_argv(1, argbuf));
  ASSERT_C_STREQ("66", process.read_argv(2, argbuf));
}

static void test_arg_passing(int argc, const char **argv) {
  RecordingProcess process;
  ASSERT_TRUE(process.start(get_durian_main(), argc, argv));
  process.complete();
  ASSERT_EQ(0, process.exit_code());
  ASSERT_EQ(argc + 1, process.read_argc());
  char argbuf[1024];
  ASSERT_C_STREQ(get_durian_main(), process.read_argv(0, argbuf));
  for (int i = 0; i < argc; i++)
    ASSERT_C_STREQ(argv[i], process.read_argv(i + 1, argbuf));
}

TEST(process_cpp, arg_with_spaces) {
  const char *argv[1] = {"foo bar baz"};
  test_arg_passing(1, argv);
}

TEST(process_cpp, multi_args_with_spaces) {
  const char *argv[3] = {"foo bar baz", "do re mi", "one two three"};
  test_arg_passing(3, argv);
}

TEST(process_cpp, quotes) {
  const char *argv[1] = {"\"hey\""};
  test_arg_passing(1, argv);
}

TEST(process_cpp, leading_slash) {
  const char *argv[1] = {"\\hey"};
  test_arg_passing(1, argv);
}

TEST(process_cpp, trailing_slash) {
  const char *argv[1] = {"hey\\"};
  test_arg_passing(1, argv);
}

TEST(process_cpp, two_trailing_slashes) {
  const char *argv[1] = {"hey\\\\"};
  test_arg_passing(1, argv);
}

TEST(process_cpp, many_trailing_slashes) {
  const char *argv[1] = {"hey\\\\\\\\\\"};
  test_arg_passing(1, argv);
}

TEST(process_cpp, quotes_and_slashes) {
  const char *argv[1] = {"\"\\h\\\"\\e\"\\\"y\"\\"};
  test_arg_passing(1, argv);
}

TEST(process_cpp, many_quotes_and_slashes) {
  const char *argv[1] = {"\"\\\"\\h\\\"\\\"\\e\"\"\\\\\"\"y\"\"\\\\"};
  test_arg_passing(1, argv);
}

TEST(process_cpp, tildes) {
  const char *argv[1] = {"^b^l\"^\"a\\^\\h^"};
  test_arg_passing(1, argv);
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
