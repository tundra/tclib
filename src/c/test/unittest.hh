//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

/// Utilities for registering C++ unit tests and building them into a program.

#ifndef _UNITTEST
#define _UNITTEST

#include "asserts.hh"
#include "io/stream.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
#include "utils/macro-inl.h"
END_C_INCLUDES

// Data that picks out a particular test or suite to run.
class unit_test_selector_t {
public:
  unit_test_selector_t() : suite_(NULL), name_(NULL), flavor_(NULL) { }

  // Returns true iff this test selector the given test.
  bool matches(const char *test_suite, const char *test_name, const char *test_flavor);

  // Sets the selector's state.
  void init(char *suite, char *name, char *flavor);

  // Cleans up if necessary.
  void dispose();

private:
  // If non-null, the test suite to run.
  char *suite_;
  // If non-null, the test case to run.
  char *name_;
  // If non-null, the test flavor to run.
  char *flavor_;
};

class TestRunInfo {
public:
  TestRunInfo() : duration_(0), count_(0) { }
  void record_run(double duration) { duration_ += duration; count_ += 1; }
  double duration() { return duration_; }
  size_t count() { return count_; }

private:
  double duration_;
  size_t count_;
};

// Information about a single test case.
class TestCaseInfo {
public:
  typedef void (*unit_test_t)();

  // Initialize a test info and tie it into the chain.
  TestCaseInfo(const char *file, const char *suite, const char *name);
  virtual ~TestCaseInfo() { }

  // Insert this info into the chain of infos.
  void insert_into_chain();

  // Run all the tests that match the given selector. Returns the time in
  // seconds it took to run the tests.
  static void run_tests(unit_test_selector_t *selector, tclib::OutStream *out,
      TestRunInfo *info);

  // Run this particular test, printing progress to the given output stream.
  // Returns the time in seconds it took to run the test.
  virtual void run(unit_test_selector_t *selector, tclib::OutStream *out,
      TestRunInfo *info) = 0;

  virtual bool matches(unit_test_selector_t *selector) = 0;

  // Check that all the tests are valid.
  static void validate_all();

  // Check that the test is correctly registered.
  void validate();

  // Flush to the time column and print the given duration.
  static void print_time(tclib::OutStream *out, size_t current_column,
      double duration);

protected:
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

  // The next test registered in the chain.
  TestCaseInfo *next;
};

class SingleTestCaseInfo : public TestCaseInfo {
public:
  // Initialize a test info and tie it into the chain.
  SingleTestCaseInfo(const char *file, const char *suite, const char *name, unit_test_t unit_test);

  virtual void run(unit_test_selector_t *selector, tclib::OutStream *out,
      TestRunInfo *info);

  virtual bool matches(unit_test_selector_t *selector);

private:
  // The unit test function that executes the test.
  unit_test_t unit_test;
};

class MultiTestCaseInfo : public TestCaseInfo {
public:
  MultiTestCaseInfo(const char *file, const char *suite, const char *name,
      size_t flavorc, const char **flavorv, unit_test_t *testv);

  virtual void run(unit_test_selector_t *selector, tclib::OutStream *out,
      TestRunInfo *info);

  virtual bool matches(unit_test_selector_t *selector);

private:
  size_t flavorc;
  const char **flavorv;
  unit_test_t *testv;
};

// Declares and registers a test case.
#define TEST(suite, name)                                                      \
  static void run_##suite##_##name();                                          \
  SingleTestCaseInfo* const test_case_info_##suite##_##name = new SingleTestCaseInfo(__FILE__, #suite, #name, run_##suite##_##name); \
  static void run_##suite##_##name()

#define __PICK_FLAVOR_NAME__(V, I) X V,
#define __BUILD_FLAVOR_FUNCTION_NAME__(V, I) &I<_ V>,

// Declares and registers a multitest, a test that can be run multiple times
// with different flavors, for whatever sense of "flavor" is appropriate for
// this test. An example of how to use this is,
//
//   MULTITEST(my_suite, my_test, size_t, ("one", 1), ("two", 2)) {
//     ...
//   }
//
// This test will be run twice, once with the Flavor variable set to 1 and once
// with it set to 2. The Flavor is a static constant so it can be used as a
// template parameter etc. To run each individual test use the names
// my_suite/my_test/one and my_suite/my_test/two. That's the only use of the
// string name.
#define MULTITEST(suite, name, type_t, ...)                                    \
  static const char *suite##_##name##_flavors[VA_ARGC(__VA_ARGS__) + 1] = {FOR_EACH_VA_ARG(__PICK_FLAVOR_NAME__, _, __VA_ARGS__) NULL}; \
  template <type_t Flavor> static void run_##suite##_##name();                 \
  static TestCaseInfo::unit_test_t suite##_##name##_tests[VA_ARGC(__VA_ARGS__) + 1] = {FOR_EACH_VA_ARG(__BUILD_FLAVOR_FUNCTION_NAME__, run_##suite##_##name, __VA_ARGS__) NULL}; \
  MultiTestCaseInfo *const test_case_info_##suite##_##name = new MultiTestCaseInfo(__FILE__, #suite, #name, VA_ARGC(__VA_ARGS__), suite##_##name##_flavors, suite##_##name##_tests); \
  template <type_t Flavor> static void run_##suite##_##name()

// Sets the global log to a value that ignores all messages. Returns the current
// log.
log_o *silence_global_log();

#endif // _UNITTEST
