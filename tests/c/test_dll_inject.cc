//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

#include "async/promise-inl.hh"
#include "helpers.hh"
#include "io/iop.hh"
#include "sync/pipe.hh"
#include "sync/process.hh"
#include "sync/thread.hh"
#include "utils/callback.hh"

BEGIN_C_INCLUDES
#include "utils/strbuf.h"
#include "utils/string-inl.h"
END_C_INCLUDES

using namespace tclib;

MULTITEST(dll_inject, exec_durian, size_t, ("clean", 0), ("inject", 1), ("inject2", 2)) {
  // Dll injections only works under msvc and not in debug mode.
  if (kIsGcc || kIsDebugCodegen)
    return;
  size_t fib_size = 85;
  byte_t fib[85];
  fib[0] = 1;
  fib[1] = 1;
  for (size_t i = 2; i < fib_size; i++)
    fib[i] = static_cast<byte_t>(fib[i-1] + fib[i-2]);
  blob_t data_in = blob_new(fib, fib_size);
  size_t i = Flavor;
  // Launch three variants of durian, which in all cases exits the value of
  // the C env variable. First time there is no injection so the exit code
  // is the initial C. Second time a dll is injected that overrides C with the
  // sum of whatever is in A and B. Third time is the same but with different
  // A and B values, just to check that the result is correspondingly
  // different.
  bool do_inject = (i > 0);
  bool other_values = (i == 2);
  NativeProcess process;
  process.set_flags(pfStartSuspendedOnWindows);
  int a = other_values ? 63 : 23;
  int b = other_values ? 42 : 83;
  process.set_env(new_c_string("A"), new_c_string(other_values ? "63" : "23"));
  process.set_env(new_c_string("B"), new_c_string(other_values ? "42" : "83"));
  process.set_env(new_c_string("C"), new_c_string("49"));
  utf8_t args[3] = {new_c_string("--quiet"), new_c_string("--exit-code-from-env"),
      new_c_string("C")};
  ASSERT_TRUE(process.start(get_durian_main(), 3, args));
  if (do_inject) {
    blob_t data_out = allocator_default_malloc(fib_size);
    NativeProcess::InjectRequest request(get_injectee_dll());
    request.set_connector(new_c_string("TestInjecteeConnect"), data_in, data_out);
    ASSERT_TRUE(process.inject_library(&request));
    uint8_t *bytes_out = static_cast<uint8_t*>(data_out.start);
    for (size_t i = 0; i < fib_size; i++)
      ASSERT_EQ(static_cast<byte_t>(fib[i] + 13), bytes_out[i]);
    allocator_default_free(data_out);
  }
  ASSERT_TRUE(process.resume());
  ProcessWaitIop wait(&process, o0());
  ASSERT_TRUE(wait.execute());
  ASSERT_EQ(do_inject ? (a + b) : 49, process.exit_code().peek_value(0));
}

TEST(dll_inject, blocking_connector) {
  // Dll injections only works under msvc and not in debug mode.
  if (kIsGcc || kIsDebugCodegen)
    return;
  NativeProcess process;

  // Start the process suspended.
  process.set_flags(pfStartSuspendedOnWindows);
  utf8_t args[3] = {new_c_string("--quiet"), new_c_string("--exit-code"),
      new_c_string("322")};
  ASSERT_TRUE(process.start(get_durian_main(), 3, args));

  // Create a channel and pass it with the inject request.
  def_ref_t<ServerChannel> server = ServerChannel::create();
  ASSERT_TRUE(server->allocate());
  utf8_t name = server->name();
  NativeProcess::InjectRequest request(get_injectee_dll());
  request.set_connector(new_c_string("TestInjecteeConnectBlocking"),
      blob_new(const_cast<char*>(name.chars), name.size), blob_empty());

  // Start the injection; it will block and wait for input on the connection
  // but this call shouldn't wait.
  ASSERT_TRUE(process.start_inject_library(&request));

  // Open the connection and write to it to let the injection run.
  ASSERT_TRUE(server->open());
  WriteIop write(server->out(), "Brilg & Spowegg", 16);
  ASSERT_TRUE(write.execute());

  // Wait for the injection to complete.
  ASSERT_TRUE(process.complete_inject_library(&request));

  // We can now close the server.
  ASSERT_TRUE(server->close());

  // The test is actually done here but we let the process finish to check that
  // it runs correctly.
  ASSERT_TRUE(process.resume());
  ProcessWaitIop wait(&process, o0());
  ASSERT_TRUE(wait.execute());
  ASSERT_EQ(322, process.exit_code().peek_value(0));
}
