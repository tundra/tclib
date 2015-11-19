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

TEST(dll_inject, exec_durian) {
  UNLESS_MSVC(return);
  for (int i = 0; i < 3; i++) {
    // Launch three variants of durian, which in all cases exits the value of
    // the C env variable. First time there is no injection so the exit code
    // is the initial C. Second time a dll is injected that overrides C with the
    // sum of whatever is in A and B. Third time is the same but with different
    // A and B values, just to check that the result is correspondingly
    // different.
    bool do_inject = (i == 1);
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
    if (do_inject)
      ASSERT_TRUE(process.inject_library(get_injectee_dll()));
    ASSERT_TRUE(process.resume());
    ProcessWaitIop wait(&process, o0());
    ASSERT_TRUE(wait.execute());
    ASSERT_EQ(do_inject ? (a + b) : 49, process.exit_code().peek_value(0));
  }
}
