//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "unittest.hh"

BEGIN_C_INCLUDES
#include "utils/crash.h"
#include "test/realtime.h"
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

void TestCaseInfo::run() {
  static const size_t kTimeColumn = 32;
  printf("- %s/%s", suite, name);
  fflush(stdout);
  double start = get_current_time_seconds();
  unit_test();
  double end = get_current_time_seconds();
  for (size_t i = strlen(suite) + strlen(name); i < kTimeColumn; i++)
    putc(' ', stdout);
  printf(" (%.3fs)\n", (end - start));
  fflush(stdout);
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

void TestCaseInfo::run_tests(unit_test_selector_t *selector) {
  TestCaseInfo *current = TestCaseInfo::chain;
  while (current != NULL) {
    if (current->matches(selector))
      current->run();
    current = current->next;
  }
}

// Run!
int main(int argc, char *argv[]) {
  install_crash_handler();
  if (argc >= 2) {
    // If there are arguments run the relevant test suites.
    for (int i = 1; i < argc; i++) {
      unit_test_selector_t selector;
      parse_test_selector(argv[i], &selector);
      TestCaseInfo::run_tests(&selector);
      dispose_test_selector(&selector);
    }
    return 0;
  } else {
    // If there are no arguments just run everything.
    unit_test_selector_t selector = {NULL, NULL};
    TestCaseInfo::run_tests(&selector);
  }
  return 0;
}
