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

static void ignore_log(log_o *log, log_entry_t *entry) {
  // ignore
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
bool TestCaseInfo::matches(unit_test_selector_t *selector) {
  if (selector->suite == NULL)
    return true;
  if (strcmp(suite, selector->suite) != 0)
    return false;
  if (selector->name == NULL)
    return true;
  return strcmp(name, selector->name) == 0;
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
  char *dupped = strdup(str);
  selector->suite = dupped;
  char *test = strchr(dupped, '/');
  if (test != NULL) {
    test[0] = 0;
    selector->name = &test[1];
  } else {
    selector->name = NULL;
  }
}

static void dispose_test_selector(unit_test_selector_t *selector) {
  free(selector->suite);
}

TestCaseInfo *TestCaseInfo::chain = NULL;

TestCaseInfo::TestCaseInfo(const char *suite, const char *name, unit_test_t unit_test) {
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

double TestCaseInfo::run_tests(unit_test_selector_t *selector, tclib::OutStream *out) {
  TestCaseInfo *current = TestCaseInfo::chain;
  double duration = 0;
  while (current != NULL) {
    if (current->matches(selector))
      duration += current->run(out);
    current = current->next;
  }
  return duration;
}

// Run!
int main(int argc, char *argv[]) {
  limited_allocator_t allocator;
  limited_allocator_install(&allocator, 10 * 1024 * 1024);
  install_crash_handler();
  tclib::OutStream *out = tclib::FileSystem::native()->std_out();
  double duration;
  if (argc >= 2) {
    // If there are arguments run the relevant test suites.
    duration = 0;
    for (int i = 1; i < argc; i++) {
      unit_test_selector_t selector;
      parse_test_selector(argv[i], &selector);
      duration += TestCaseInfo::run_tests(&selector, out);
      dispose_test_selector(&selector);
    }
  } else {
    // If there are no arguments just run everything.
    unit_test_selector_t selector = {NULL, NULL};
    duration = TestCaseInfo::run_tests(&selector, out);
  }
  size_t column = out->printf("  all tests passed");
  TestCaseInfo::print_time(out, column, duration);
  // Return a successful error code only if there were no allocator leaks.
  return limited_allocator_uninstall(&allocator) ? 0 : 1;
}
