//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "unittest.hh"
#include "io/file.hh"

BEGIN_C_INCLUDES
#include "test/realtime.h"
#include "utils/alloc.h"
#include "utils/crash.h"
END_C_INCLUDES

#ifdef IS_GCC
extern char *strdup(const char*);
#endif

IMPLEMENTATION(silent_log_o, log_o);

static bool ignore_log(log_o *log, log_entry_t *entry) {
  // ignore
  return true;
}

VTABLE(silent_log_o, log_o) { ignore_log };

log_o *silence_global_log() {
  static log_o kSilentLog;
  static log_o *silent_log = NULL;
  if (silent_log == NULL) {
    VTABLE_INIT(silent_log_o, &kSilentLog);
    silent_log = &kSilentLog;
  }
  return set_global_log(silent_log);
}

// Match the given suite and test name against the given unit test data, return
// true iff the test should be run.
bool unit_test_selector_t::matches(const char *test_suite, const char *test_name) {
  if (suite_ == NULL)
    return true;
  if (strcmp(suite_, test_suite) != 0)
    return false;
  if (name_ == NULL)
    return true;
  return strcmp(name_, test_name) == 0;
}

void unit_test_selector_t::init(char *suite, char *name) {
  suite_ = suite;
  name_ = name;
}

void unit_test_selector_t::dispose() {
  free(suite_);
  suite_ = NULL;
  name_ = NULL;
}

void TestCaseInfo::print_time(tclib::OutStream *out, size_t current_column,
    double duration) {
  for (size_t i = current_column; i < kTimeColumn; i++)
    out->printf(" ");
  out->printf("(%.3fs)\n", duration);
  out->flush();
}

double TestCaseInfo::run(tclib::OutStream *out) {
  size_t column = out->printf("- %s/%s", suite, name);
  out->flush();
  double start = get_current_time_seconds();
  unit_test();
  double end = get_current_time_seconds();
  double duration = end - start;
  print_time(out, column, duration);
  return duration;
}

static void parse_test_selector(const char *str, unit_test_selector_t *selector) {
  char *name = NULL;
  char *suite = strdup(str);
  char *test = strchr(suite, '/');
  if (test != NULL) {
    test[0] = 0;
    name = &test[1];
  } else {
    name = NULL;
  }
  selector->init(suite, name);
}

TestCaseInfo *TestCaseInfo::chain = NULL;

TestCaseInfo::TestCaseInfo(const char *file, const char *suite, const char *name,
    unit_test_t unit_test) {
  this->file = file;
  this->suite = suite;
  this->name = name;
  this->unit_test = unit_test;
  this->next = NULL;
  // Add this test at the end of the chain. This makes setting up the tests
  // quadratic in the number of cases but on the other hand it's a simple way
  // to ensure that they're run in declaration order and there shouldn't be
  // too many tests.
  if (TestCaseInfo::chain == NULL) {
    TestCaseInfo::chain = this;
  } else {
    TestCaseInfo *current = TestCaseInfo::chain;
    while (current->next != NULL)
      current = current->next;
    current->next = this;
  }
}

void TestCaseInfo::validate() {
  // Look for the test case marker in the filename.
  const char *marker = "test_";
  const char *basename_start = strstr(this->file, marker);
  if (basename_start == NULL)
    FATAL("Test file %s doesn't start with '%s'", this->file, marker);
  // Check that what comes after the marker matches the test suite.
  const char *basename_suffix = basename_start + strlen(marker);
  if (strstr(basename_suffix, this->suite) != basename_suffix)
    FATAL("Test %s/%s defined in file %s", this->suite, this->name, this->file);
}

double TestCaseInfo::run_tests(unit_test_selector_t *selector, tclib::OutStream *out) {
  TestCaseInfo *current = TestCaseInfo::chain;
  double duration = 0;
  while (current != NULL) {
    if (selector->matches(current->suite, current->name))
      duration += current->run(out);
    current = current->next;
  }
  return duration;
}

void TestCaseInfo::validate_all() {
  TestCaseInfo *current = TestCaseInfo::chain;
  while (current != NULL) {
    current->validate();
    current = current->next;
  }
}

// Run!
int main(int argc, char *argv[]) {
  limited_allocator_t allocator;
  limited_allocator_install(&allocator, 100 * 1024 * 1024);
  install_crash_handler();
  tclib::OutStream *out = tclib::FileSystem::native()->std_out();
  double duration;
  TestCaseInfo::validate_all();
  if (argc >= 2) {
    // If there are arguments run the relevant test suites.
    duration = 0;
    for (int i = 1; i < argc; i++) {
      unit_test_selector_t selector;
      parse_test_selector(argv[i], &selector);
      duration += TestCaseInfo::run_tests(&selector, out);
      selector.dispose();
    }
  } else {
    // If there are no arguments just run everything.
    unit_test_selector_t selector;
    duration = TestCaseInfo::run_tests(&selector, out);
  }
  size_t column = out->printf("  all tests passed");
  TestCaseInfo::print_time(out, column, duration);
  // Return a successful error code only if there were no allocator leaks.
  return limited_allocator_uninstall(&allocator) ? 0 : 1;
}
