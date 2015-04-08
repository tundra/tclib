//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

/// Utilities for registering C++ unit tests and building them into a program.

#ifndef _UNITTEST
#define _UNITTEST

#include "asserts.hh"
#include "io/stream.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
END_C_INCLUDES

// Data that picks out a particular test or suite to run.
class unit_test_selector_t {
public:
  unit_test_selector_t() : suite_(NULL), name_(NULL) { }

  // Returns true iff this test selector the given test.
  bool matches(const char *test_suite, const char *test_name);

  // Sets the selector's state.
  void init(char *suite, char *name);

  // Cleans up if necessary.
  void dispose();

private:
  // If non-null, the test suite to run.
  char *suite_;
  // If non-null, the test case to run.
  char *name_;
};

// Information about a single test case.
class TestCaseInfo {
public:
  // The type of a unit test function.
  typedef void (*unit_test_t)();

  // Initialize a test info and tie it into the chain.
  TestCaseInfo(const char *file, const char *suite, const char *name, unit_test_t unit_test);

  // Run all the tests that match the given selector. Returns the time in
  // seconds it took to run the tests.
  static double run_tests(unit_test_selector_t *selector, tclib::OutStream *out);

  // Run this particular test, printing progress to the given output stream.
  // Returns the time in seconds it took to run the test.
  double run(tclib::OutStream *out);

  // Check that all the tests are valid.
  static void validate_all();

  // Check that the test is correctly registered.
  void validate();

  // Flush to the time column and print the given duration.
  static void print_time(tclib::OutStream *out, size_t current_column,
      double duration);

private:
  // The head of the chain of registered tests.
  static TestCaseInfo *chain;

  // How far to the right to flush the duration when printing progress.
  static const size_t kTimeColumn = 36;

  // Name of the file that defines the test. Used for validation.
  const char *file;

  // Suite and name; the only significance of the suite is that it gives you a
  // different criterium for which tests to run, there's not setup/teardown etc.
  const char *suite;
  const char *name;

  // The unit test function that executes the test.
  unit_test_t unit_test;

  // The next test registered in the chain.
  TestCaseInfo *next;
};

// Declares and registers a test case.
#define TEST(suite, name)                                                      \
  void run_##suite##_##name();                                                 \
  TestCaseInfo* const test_case_info_##suite##_##name = new TestCaseInfo(__FILE__, #suite, #name, run_##suite##_##name); \
  void run_##suite##_##name()

// Sets the global log to a value that ignores all messages. Returns the current
// log.
log_o *silence_global_log();

#endif // _UNITTEST
