//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "async/promise-inl.hh"
#include "io/iop.hh"
#include "sync/pipe.hh"
#include "sync/process.hh"
#include "test/unittest.hh"
#include "utils/callback.hh"

#include "sync/thread.hh"

BEGIN_C_INCLUDES
#include "utils/strbuf.h"
#include "utils/string-inl.h"
END_C_INCLUDES

using namespace tclib;

TEST(process_cpp, exec_missing) {
  NativeProcess process;
  ASSERT_TRUE(process.start(new_c_string("test_process_cpp_exec_fail_missing_executable"),
      0, NULL));
  ProcessWaitIop wait(&process, o0());
  ASSERT_TRUE(wait.execute());
  ASSERT_TRUE(process.exit_code().peek_value(0) != 0);
}

// Returns the path to the durian executable.
static utf8_t get_durian_main() {
  const char *result = getenv("DURIAN_MAIN");
  ASSERT_TRUE(result != NULL);
  return new_c_string(result);
}

// A process that records interaction.
class RecordingProcess : public NativeProcess {
public:
  RecordingProcess();
  ~RecordingProcess();

  // Sets the data to write on the process' stdin.
  void set_stdin_data(const char *data);

  // Waits for the process to complete, meanwhile capturing output.
  void complete();

  // Extracts the recorded argc from the process' output, if no argc can be
  // found returns -1.
  int read_argc();

  // Returns the value of the index'th entry in the process' argv. The given
  // char buffer is scratch memory to use to store the result. If no argument
  // is found returns NULL.
  const char *read_argv(int index, char *scratch, size_t scratch_size);

  // Returns the value of the environment variable with the given name. The
  // given char buffer is scratch memory to use to store the result. If the env
  // is not found returns NULL.
  const char *read_env(const char *name, char *scratch, size_t scratch_size);

  // Returns the value of the getenv output printed by the process. The given
  // char buffer is scratch memory to use to store the result. If the env is not
  // found returns NULL.
  const char *read_getenv(const char *name, char *scratch, size_t outsize);

  // Returns the raw stderr output as a string.
  const char *err() { return stderr_str_; }

public:
  // Iterate through the process' standard output and, for each line, scan the
  // line for the given format using scanf. The first time the scan is
  // successful this function returns.
  bool scanf_lines(const char *fmt, void **ptrv, size_t ptrc);

  NativePipe stdin_pipe_;
  NativePipe stdout_pipe_;
  NativePipe stderr_pipe_;
  string_buffer_t stdout_buf_;
  string_buffer_t stderr_buf_;
  blob_t stdin_data_;
  const char *stdout_str_;
  const char *stderr_str_;
};

RecordingProcess::RecordingProcess()
  : stdin_data_(blob_empty())
  , stdout_str_(NULL)
  , stderr_str_(NULL) {
  ASSERT_TRUE(stdin_pipe_.open(NativePipe::pfInherit));
  set_stream(siStdin, stdin_pipe_.redirect(pdIn));
  ASSERT_TRUE(stdout_pipe_.open(NativePipe::pfInherit));
  set_stream(siStdout, stdout_pipe_.redirect(pdOut));
  string_buffer_init(&stdout_buf_);
  ASSERT_TRUE(stderr_pipe_.open(NativePipe::pfInherit));
  set_stream(siStderr, stderr_pipe_.redirect(pdOut));
  string_buffer_init(&stderr_buf_);
}

RecordingProcess::~RecordingProcess() {
  string_buffer_dispose(&stdout_buf_);
  string_buffer_dispose(&stderr_buf_);
}

void RecordingProcess::set_stdin_data(const char *data) {
  stdin_data_ = blob_new(const_cast<char*>(data), strlen(data));
}


void RecordingProcess::complete() {
  static const size_t kStdinIndex = 0;
  static const size_t kStdoutIndex = 1;
  static const size_t kStderrIndex = 2;
  IopGroup group;
  size_t stdin_cursor = 0;
  WriteIop write_stdin(stdin_pipe_.out(), stdin_data_.start, stdin_data_.size,
      u2o(kStdinIndex));
  group.schedule(&write_stdin);
  char stdout_buf[256];
  ReadIop read_stdout(stdout_pipe_.in(), stdout_buf, 256, u2o(kStdoutIndex));
  group.schedule(&read_stdout);
  char stderr_buf[256];
  ReadIop read_stderr(stderr_pipe_.in(), stderr_buf, 256, u2o(kStderrIndex));
  group.schedule(&read_stderr);
  while (group.has_pending()) {
    Iop *next = NULL;
    ASSERT_TRUE(group.wait_for_next(Duration::unlimited(), &next));
    uint64_t index = o2u(next->extra());
    if (index == kStdinIndex) {
      ASSERT_TRUE(write_stdin.has_succeeded());
      stdin_cursor += write_stdin.bytes_written();
      if (stdin_cursor < stdin_data_.size) {
        write_stdin.recycle(static_cast<byte_t*>(stdin_data_.start) + stdin_cursor,
            stdin_data_.size - stdin_cursor);
      } else {
        ASSERT_TRUE(stdin_pipe_.out()->close());
      }
    } else {
      ReadIop *iop;
      char *buf;
      string_buffer_t *strbuf;
      if (index == kStdoutIndex) {
        iop = &read_stdout;
        buf = stdout_buf;
        strbuf = &stdout_buf_;
      } else {
        ASSERT_EQ(kStderrIndex, index);
        iop = &read_stderr;
        buf = stderr_buf;
        strbuf = &stderr_buf_;
      }
      ASSERT_TRUE(iop->has_succeeded());
      string_buffer_append(strbuf, new_string(buf, iop->bytes_read()));
      if (!iop->at_eof())
        iop->recycle();
    }
  }
  // We know the process has completed but still need to call wait for the
  // state to be updated.
  ProcessWaitIop wait_iop(this, o0());
  ASSERT_TRUE(wait_iop.execute());
  stdout_str_ = string_buffer_flush(&stdout_buf_).chars;
  stderr_str_ = string_buffer_flush(&stderr_buf_).chars;
}

int RecordingProcess::read_argc() {
  int result = -1;
  void *ptrs[1] = {&result};
  ASSERT_TRUE(scanf_lines("ARGC: {%i}", ptrs, 1));
  return result;
}

const char *RecordingProcess::read_argv(int index, char *out, size_t outsize) {
  ASSERT_REL(outsize, >=, 1024);
  char fmt[256];
  sprintf(fmt, "ARGV[%i]: {%%1024[^}]}", index);
  return scanf_lines(fmt, (void**) &out, 1) ? out : NULL;
}

const char *RecordingProcess::read_env(const char *key, char *out, size_t outsize) {
  ASSERT_REL(outsize, >=, 1024);
  char fmt[256];
  sprintf(fmt, "ENV: {%s=%%1024[^}]}", key);
  return scanf_lines(fmt, (void**) &out, 1) ? out : NULL;
}

const char *RecordingProcess::read_getenv(const char *key, char *out, size_t outsize) {
  ASSERT_REL(outsize, >=, 1024);
  char fmt[256];
  sprintf(fmt, "GETENV(%s): {%%1024[^}]}", key);
  return scanf_lines(fmt, (void**) &out, 1) ? out : NULL;
}

bool RecordingProcess::scanf_lines(const char *fmt, void **ptrv, size_t ptrc) {
  utf8_t format = new_c_string(fmt);
  scanf_conversion_t convs[8];
  ASSERT_EQ(ptrc, string_scanf_analyze_conversions(format, convs, 8));
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
    if (current != NULL) {
      int64_t scanned = string_scanf(format, new_c_string(current), convs, ptrc, ptrv);
      if (scanned == (int64_t) ptrc)
        return true;
    }
  }
  return false;
}

TEST(process_cpp, return_value) {
  RecordingProcess process;
  utf8_t argv[2] = {new_c_string("--exit-code"), new_c_string("66")};
  ASSERT_TRUE(process.start(get_durian_main(), 2, argv));
  NativeThread::sleep(Duration::seconds(1));
  process.complete();
  ASSERT_EQ(66, process.exit_code().peek_value(0));
  ASSERT_EQ(3, process.read_argc());
  char argbuf[1024];
  ASSERT_C_STREQ(get_durian_main().chars, process.read_argv(0, argbuf, 1024));
  ASSERT_C_STREQ("--exit-code", process.read_argv(1, argbuf, 1024));
  ASSERT_C_STREQ("66", process.read_argv(2, argbuf, 1024));
}

static void test_arg_passing(int argc, utf8_t *argv) {
  RecordingProcess process;
  ASSERT_TRUE(process.start(get_durian_main(), argc, argv));
  process.complete();
  ASSERT_EQ(0, process.exit_code().peek_value(100));
  ASSERT_EQ(argc + 1, process.read_argc());
  char argbuf[1024];
  ASSERT_C_STREQ(get_durian_main().chars, process.read_argv(0, argbuf, 1024));
  for (int i = 0; i < argc; i++)
    ASSERT_C_STREQ(argv[i].chars, process.read_argv(i + 1, argbuf, 1024));
}

TEST(process_cpp, arg_spaces) {
  utf8_t argv[1] = {new_c_string("foo bar baz")};
  test_arg_passing(1, argv);
}

TEST(process_cpp, multi_args_with_spaces) {
  utf8_t argv[3] = {new_c_string("foo bar baz"), new_c_string("do re mi"),
      new_c_string("one two three")};
  test_arg_passing(3, argv);
}

TEST(process_cpp, quotes) {
  utf8_t argv[1] = {new_c_string("\"hey\"")};
  test_arg_passing(1, argv);
}

TEST(process_cpp, leading_slash) {
  utf8_t argv[1] = {new_c_string("\\hey")};
  test_arg_passing(1, argv);
}

TEST(process_cpp, trailing_slash) {
  utf8_t argv[1] = {new_c_string("hey\\")};
  test_arg_passing(1, argv);
}

TEST(process_cpp, two_trailing_slashes) {
  utf8_t argv[1] = {new_c_string("hey\\\\")};
  test_arg_passing(1, argv);
}

TEST(process_cpp, many_trailing_slashes) {
  utf8_t argv[1] = {new_c_string("hey\\\\\\\\\\")};
  test_arg_passing(1, argv);
}

TEST(process_cpp, quotes_and_slashes) {
  utf8_t argv[1] = {new_c_string("\"\\h\\\"\\e\"\\\"y\"\\")};
  test_arg_passing(1, argv);
}

TEST(process_cpp, many_quotes_and_slashes) {
  utf8_t argv[1] = {new_c_string("\"\\\"\\h\\\"\\\"\\e\"\"\\\\\"\"y\"\"\\\\")};
  test_arg_passing(1, argv);
}

TEST(process_cpp, caret) {
  utf8_t argv[1] = {new_c_string("^b^l\"^\"a\\^\\h^")};
  test_arg_passing(1, argv);
}

TEST(process_cpp, env_simple) {
  RecordingProcess process;
  process.set_env(new_c_string("FOO"), new_c_string("bar"));
  ASSERT_TRUE(process.start(get_durian_main(), 0, NULL));
  process.complete();
  ASSERT_EQ(0, process.exit_code().peek_value(100));
  ASSERT_EQ(1, process.read_argc());
  char envbuf[1024];
  ASSERT_C_STREQ("bar", process.read_env("FOO", envbuf, 1024));
}

class binding_t {
public:
  binding_t(const char *key, const char *value, bool is_eq_generated = false)
    : key_(new_c_string(key))
    , value_(new_c_string(value))
    , is_eq_generated_(is_eq_generated) { }

  // The name to bind to.
  utf8_t key() { return key_; }

  // The value to bind.
  utf8_t value() { return value_; }

  // Is this not an explicit binding but one generated by having an eq sign
  // somewhere in the key or value?
  bool is_eq_generated() { return is_eq_generated_; }

private:
  utf8_t key_;
  utf8_t value_;
  bool is_eq_generated_;
};

static void test_env_passing(size_t in_envc, binding_t *in_envv, size_t out_envc,
    binding_t *out_envv) {
  RecordingProcess process;
  for (size_t i = 0; i < in_envc; i++)
    process.set_env(in_envv[i].key(), in_envv[i].value());
  // We're interested both in how the environ array looks but also what gets
  // returned from getenv so for each binding we ask the program to print the
  // result of getenv.
  size_t argc = out_envc * 2;
  utf8_t *argv = new utf8_t[argc];
  for (size_t i = 0; i < out_envc; i++) {
    argv[2 * i] = new_c_string("--getenv");
    argv[2 * i + 1] = out_envv[i].key();
  }
  ASSERT_TRUE(process.start(get_durian_main(), argc, argv));
  delete[] argv;
  process.complete();
  ASSERT_EQ(0, process.exit_code().peek_value(100));
  ASSERT_EQ(argc + 1, process.read_argc());
  char argbuf[1024];
  ASSERT_C_STREQ(get_durian_main().chars, process.read_argv(0, argbuf, 1024));
  for (size_t i = 0; i < out_envc; i++) {
    binding_t out = out_envv[i];
    if (IF_MACH(!out.is_eq_generated(), true)) {
      // Mach doesn't have the same behavior for eq-generated bindings so don't
      // test that there.
      ASSERT_C_STREQ(out.value().chars, process.read_env(out.key().chars, argbuf, 1024));
      ASSERT_C_STREQ(out.value().chars, process.read_getenv(out.key().chars, argbuf, 1024));
    }
  }
  ASSERT_C_STREQ("", process.err());
}

#define ALEN(A) (sizeof(A) / sizeof(*(A)))

TEST(process_cpp, env_multiple) {
  binding_t envv[2] = {binding_t("FOO", "foo"), binding_t("BAR", "bar")};
  test_env_passing(ALEN(envv), envv, ALEN(envv), envv);
}

TEST(process_cpp, env_value_eq) {
  binding_t in_envv[1] = {binding_t("FOO", "fo=o")};
  binding_t out_envv[2] = {binding_t("FOO", "fo=o"), binding_t("FOO=fo", "o", true)};
  test_env_passing(ALEN(in_envv), in_envv, ALEN(out_envv), out_envv);
}

TEST(process_cpp, env_value_quote_eq) {
  // In the env, no one can hear you quote.
  binding_t in_envv[1] = {binding_t("FOO", "fo\\=o")};
  binding_t out_envv[2] = {binding_t("FOO", "fo\\=o"), binding_t("FOO=fo\\", "o", true)};
  test_env_passing(ALEN(in_envv), in_envv, ALEN(out_envv), out_envv);
}

TEST(process_cpp, env_key_eq) {
  binding_t in_envv[1] = {binding_t("FO=O", "foo")};
  binding_t out_envv[2] = {binding_t("FO=O", "foo", true), binding_t("FO", "O=foo")};
  test_env_passing(ALEN(in_envv), in_envv, ALEN(out_envv), out_envv);
}

TEST(process_cpp, env_key_quote) {
  binding_t in_envv[1] = {binding_t("\"F\'O\\O\"", "foo")};
  test_env_passing(ALEN(in_envv), in_envv, ALEN(in_envv), in_envv);
}

TEST(process_cpp, env_key_space) {
  binding_t in_envv[1] = {binding_t("F O O", "foo")};
  test_env_passing(ALEN(in_envv), in_envv, ALEN(in_envv), in_envv);
}

TEST(process_cpp, env_simple_override) {
  binding_t in_envv[2] = {binding_t("FOO", "foo"), binding_t("FOO", "bar")};
  binding_t out_envv[1] = {binding_t("FOO", "bar")};
  test_env_passing(ALEN(in_envv), in_envv, ALEN(out_envv), out_envv);
}

TEST(process_cpp, env_multi_override) {
  binding_t in_envv[5] = {binding_t("FOO", "1"), binding_t("BAR", "a"), binding_t("FOO", "2"), binding_t("BAR", "b"), binding_t("FOO", "3")};
  binding_t out_envv[2] = {binding_t("FOO", "3"), binding_t("BAR", "b")};
  test_env_passing(ALEN(in_envv), in_envv, ALEN(out_envv), out_envv);
}

#define STDERR_MESSAGE "I am process, hear me stderr!"

TEST(process_cpp, stderr) {
  RecordingProcess process;
  utf8_t argv[2] = {new_c_string("--print-stderr"), new_c_string(STDERR_MESSAGE)};
  ASSERT_TRUE(process.start(get_durian_main(), 2, argv));
  process.complete();
  ASSERT_EQ(0, process.exit_code().peek_value(100));
  ASSERT_EQ(3, process.read_argc());
  // Printing adds a newline.
  ASSERT_C_STREQ(STDERR_MESSAGE, process.err());
}

#define STDIN_MESSAGE "In through the one pipe and out of the other."

TEST(process_cpp, stdin) {
  RecordingProcess process;
  process.set_stdin_data(STDIN_MESSAGE);
  utf8_t  argv[1] = {new_c_string("--echo-stdin")};
  ASSERT_TRUE(process.start(get_durian_main(), 1, argv));
  process.complete();
  ASSERT_EQ(0, process.exit_code().peek_value(100));
  ASSERT_EQ(2, process.read_argc());
  // Printing adds a newline.
  ASSERT_C_STREQ(STDIN_MESSAGE, process.err());
}

// Processes are a bit more expensive on windows.
#define kProcessCount IF_MSVC(64, 256)

static bool async_exit(NativeSemaphore *callback_count, int exit_code) {
  ASSERT_EQ(88, exit_code);
  ASSERT_TRUE(callback_count->release());
  return true;
}

TEST(process_cpp, terminate_avalanche) {
  // Spin off N children all eventually blocking to read from stdin.
  NativeProcess processes[kProcessCount];
  NativePipe stdins[kProcessCount];
  NativeSemaphore callback_count(0);
  ASSERT_TRUE(callback_count.initialize());
  utf8_t argv[4] = {new_c_string("--exit-code"), new_c_string("88"),
      new_c_string("--echo-stdin"), new_c_string("--quiet")};
  for (size_t i = 0; i < kProcessCount; i++) {
    ASSERT_TRUE(stdins[i].open(NativePipe::pfInherit));
    processes[i].set_stream(siStdin, stdins[i].redirect(pdIn));
    NativeProcess *process = &processes[i];
    ASSERT_TRUE(process->start(get_durian_main(), 4, argv));
    promise_t<int> exit_code = process->exit_code();
    ASSERT_FALSE(exit_code.is_settled());
    exit_code.then(new_callback(async_exit, &callback_count));
  }

  // Sleep a little bit to let them all start.
  ASSERT_TRUE(NativeThread::sleep(Duration::millis(50)));

  // Check that as far as we know they're still all running.
  for (size_t i = 0; i < kProcessCount; i++)
    ASSERT_FALSE(processes[i].wait_sync(Duration::instant()));

  ASSERT_FALSE(callback_count.acquire(Duration::instant()));

  // Close the stdins; this should cause the processes to exit.
  for (size_t i = 0; i < kProcessCount; i++)
    ASSERT_TRUE(stdins[i].out()->close());

  // Now all the processes should terminate. If this deadlocks it's typically
  // a sign that a process termination gets lost in the flood; that's what this
  // test is for.
  for (size_t i = 0; i < kProcessCount; i++) {
    ASSERT_TRUE(processes[i].wait_sync());
    promise_t<int> exit_code = processes[i].exit_code();
    ASSERT_TRUE(exit_code.is_settled());
    ASSERT_EQ(88, exit_code.peek_value(0));
    ASSERT_TRUE(callback_count.acquire());
  }
}
