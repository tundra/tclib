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

  // Returns the value of the environment variable with the given name. The
  // given char buffer is scratch memory to use to store the result. If the env
  // is not found returns NULL.
  const char *read_env(const char *name, char *scratch);

  // Returns the value of the getenv output printed by the process. The given
  // char buffer is scratch memory to use to store the result. If the env is not
  // found returns NULL.
  const char *read_getenv(const char *name, char *scratch);

  // Returns the raw stderr output as a string.
  const char *err() { return stderr_str_; }

public:
  // Iterate through the process' standard output and, for each line, invoke the
  // given callback. The first time the scan is successful (returns true) this
  // function returns.
  bool for_each_stdout_line(callback_t<bool(const char *line)> callback);

  NativePipe stdout_pipe_;
  NativePipe stderr_pipe_;
  PipeRedirect stdout_redirect_;
  PipeRedirect stderr_redirect_;
  string_buffer_t stdout_buf_;
  string_buffer_t stderr_buf_;
  const char *stdout_str_;
  const char *stderr_str_;
};

RecordingProcess::RecordingProcess()
  : stdout_redirect_(&stdout_pipe_, PipeRedirect::pdOut)
  , stderr_redirect_(&stderr_pipe_, PipeRedirect::pdOut)
  , stdout_str_(NULL)
  , stderr_str_(NULL) {
  ASSERT_TRUE(stdout_pipe_.open(NativePipe::pfInherit));
  set_stdout(&stdout_redirect_);
  string_buffer_init(&stdout_buf_);
  ASSERT_TRUE(stderr_pipe_.open(NativePipe::pfInherit));
  set_stderr(&stderr_redirect_);
  string_buffer_init(&stderr_buf_);
}

RecordingProcess::~RecordingProcess() {
  string_buffer_dispose(&stdout_buf_);
  string_buffer_dispose(&stderr_buf_);
}

void RecordingProcess::complete() {
  IopGroup group;
  char stdout_buf[256];
  ReadIop read_stdout(stdout_pipe_.in(), stdout_buf, 256);
  group.schedule(&read_stdout);
  char stderr_buf[256];
  ReadIop read_stderr(stderr_pipe_.in(), stderr_buf, 256);
  group.schedule(&read_stderr);
  for (int live_count = 2; live_count > 0;) {
    size_t index = 0;
    ASSERT_TRUE(group.wait_for_next(&index));
    ReadIop *iop;
    char *buf;
    string_buffer_t *strbuf;
    if (index == 0) {
      iop = &read_stdout;
      buf = stdout_buf;
      strbuf = &stdout_buf_;
    } else {
      ASSERT_EQ(1, index);
      iop = &read_stderr;
      buf = stderr_buf;
      strbuf = &stderr_buf_;
    }
    string_buffer_append(strbuf, new_string(buf, iop->bytes_read()));
    if (iop->at_eof()) {
      live_count--;
    } else {
      iop->recycle();
    }
  }
  // We know the process has completed but still need to call wait for the
  // state to be updated.
  wait();
  stdout_str_ = string_buffer_flush(&stdout_buf_).chars;
  stderr_str_ = string_buffer_flush(&stderr_buf_).chars;
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

static bool scan_env(const char *key, char *out, const char *line) {
  char fmt[256];
  sprintf(fmt, "ENV: {%s=%%[^}]}", key);
  return sscanf(line, fmt, out) > 0;
}

const char *RecordingProcess::read_env(const char *key, char *out) {
  return for_each_stdout_line(new_callback(scan_env, key, out))
    ? out
    : NULL;
}

static bool scan_getenv(const char *key, char *out, const char *line) {
  char fmt[256];
  sprintf(fmt, "GETENV(%s): {%%[^}]}", key);
  return sscanf(line, fmt, out) > 0;
}

const char *RecordingProcess::read_getenv(const char *key, char *out) {
  return for_each_stdout_line(new_callback(scan_getenv, key, out))
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

TEST(process_cpp, arg_spaces) {
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

TEST(process_cpp, caret) {
  const char *argv[1] = {"^b^l\"^\"a\\^\\h^"};
  test_arg_passing(1, argv);
}

TEST(process_cpp, env_simple) {
  RecordingProcess process;
  process.set_env("FOO", "bar");
  ASSERT_TRUE(process.start(get_durian_main(), 0, NULL));
  process.complete();
  ASSERT_EQ(0, process.exit_code());
  ASSERT_EQ(1, process.read_argc());
  char envbuf[1024];
  ASSERT_C_STREQ("bar", process.read_env("FOO", envbuf));
}

// Inline environment binding.
typedef struct {
  const char *key;
  const char *value;
} binding_t;

static void test_env_passing(size_t in_envc, binding_t *in_envv, size_t out_envc,
    binding_t *out_envv) {
  RecordingProcess process;
  for (size_t i = 0; i < in_envc; i++)
    process.set_env(in_envv[i].key, in_envv[i].value);
  // We're interested both in how the environ array looks but also what gets
  // returned from getenv so for each binding we ask the program to print the
  // result of getenv.
  size_t argc = out_envc * 2;
  const char **argv = new const char *[argc];
  for (size_t i = 0; i < out_envc; i++) {
    argv[2 * i] = "--getenv";
    argv[2 * i + 1] = out_envv[i].key;
  }
  ASSERT_TRUE(process.start(get_durian_main(), argc, argv));
  delete[] argv;
  process.complete();
  ASSERT_EQ(0, process.exit_code());
  ASSERT_EQ(argc + 1, process.read_argc());
  char argbuf[1024];
  ASSERT_C_STREQ(get_durian_main(), process.read_argv(0, argbuf));
  for (size_t i = 0; i < out_envc; i++) {
    ASSERT_C_STREQ(out_envv[i].value, process.read_env(out_envv[i].key, argbuf));
    ASSERT_C_STREQ(out_envv[i].value, process.read_getenv(out_envv[i].key, argbuf));
  }
  ASSERT_C_STREQ("", process.err());
}

#define ALEN(A) (sizeof(A) / sizeof(*(A)))

TEST(process_cpp, env_multiple) {
  binding_t envv[2] = {{"FOO", "foo"}, {"BAR", "bar"}};
  test_env_passing(ALEN(envv), envv, ALEN(envv), envv);
}

TEST(process_cpp, env_value_eq) {
  binding_t in_envv[1] = {{"FOO", "fo=o"}};
  binding_t out_envv[2] = {{"FOO", "fo=o"}, {"FOO=fo", "o"}};
  test_env_passing(ALEN(in_envv), in_envv, ALEN(out_envv), out_envv);
}

TEST(process_cpp, env_value_quote_eq) {
  // In the env, no one can hear you quote.
  binding_t in_envv[1] = {{"FOO", "fo\\=o"}};
  binding_t out_envv[2] = {{"FOO", "fo\\=o"}, {"FOO=fo\\", "o"}};
  test_env_passing(ALEN(in_envv), in_envv, ALEN(out_envv), out_envv);
}

TEST(process_cpp, env_key_eq) {
  binding_t in_envv[1] = {{"FO=O", "foo"}};
  binding_t out_envv[2] = {{"FO=O", "foo"}, {"FO", "O=foo"}};
  test_env_passing(ALEN(in_envv), in_envv, ALEN(out_envv), out_envv);
}

TEST(process_cpp, env_key_quote) {
  binding_t in_envv[1] = {{"\"F\'O\\O\"", "foo"}};
  test_env_passing(ALEN(in_envv), in_envv, ALEN(in_envv), in_envv);
}

TEST(process_cpp, env_key_space) {
  binding_t in_envv[1] = {{"F O O", "foo"}};
  test_env_passing(ALEN(in_envv), in_envv, ALEN(in_envv), in_envv);
}

TEST(process_cpp, env_simple_override) {
  binding_t in_envv[2] = {{"FOO", "foo"}, {"FOO", "bar"}};
  binding_t out_envv[1] = {{"FOO", "bar"}};
  test_env_passing(ALEN(in_envv), in_envv, ALEN(out_envv), out_envv);
}

TEST(process_cpp, env_multi_override) {
  binding_t in_envv[5] = {{"FOO", "1"}, {"BAR", "a"}, {"FOO", "2"}, {"BAR", "b"}, {"FOO", "3"}};
  binding_t out_envv[2] = {{"FOO", "3"}, {"BAR", "b"}};
  test_env_passing(ALEN(in_envv), in_envv, ALEN(out_envv), out_envv);
}

#define MESSAGE "I am process, hear me stderr!"

TEST(process_cpp, stderr) {
  IF_MSVC(return,);
  RecordingProcess process;
  const char *argv[2] = {"--print-stderr", MESSAGE};
  ASSERT_TRUE(process.start(get_durian_main(), 2, argv));
  process.complete();
  ASSERT_EQ(0, process.exit_code());
  ASSERT_EQ(3, process.read_argc());
  // Printing adds a newline.
  ASSERT_C_STREQ(MESSAGE "\n", process.err());
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
