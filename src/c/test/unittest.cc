//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "unittest.hh"
#include "io/file.hh"

BEGIN_C_INCLUDES
#include "test/realtime.h"
#include "utils/alloc.h"
#include "utils/crash.h"
#include "utils/lifetime.h"
END_C_INCLUDES

#ifdef IS_GCC
extern char *strdup(const char*);
#endif

#define kTotalMarker "="
#define kTestMarker "-"

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
bool unit_test_selector_t::matches(const char *test_suite, const char *test_name,
    const char *test_flavor) {
  if (suite_ == NULL)
    return true;
  if (strcmp(suite_, test_suite) != 0)
    return false;
  if (name_ == NULL)
    return true;
  if (strcmp(name_, test_name) != 0)
    return false;
  if (test_flavor == NULL || flavor_ == NULL)
    return true;
  return strcmp(flavor_, test_flavor) == 0;
}

void unit_test_selector_t::init(char *suite, char *name, char *flavor) {
  suite_ = suite;
  name_ = name;
  flavor_ = flavor;
}

void unit_test_selector_t::dispose() {
  free(suite_);
  suite_ = NULL;
  name_ = NULL;
  flavor_ = NULL;
}

void TestCaseInfo::print_time(tclib::OutStream *out, size_t current_column,
    double duration, TestRunHandle *run_handle) {
  for (size_t i = current_column; i < kTimeColumn; i++)
    out->printf(" ");
  if (run_handle != NULL && run_handle->was_skipped()) {
    out->printf("(------)");
    const char *why = run_handle->why_skipped();
    if (why != NULL)
      out->printf(" // %s", why);
    out->printf("\n");
  } else {
    out->printf("(%.3fs)\n", duration);
  }
  out->flush();
}

void SingleTestCaseInfo::run(unit_test_selector_t *selector, tclib::OutStream *out,
    TestRunInfo *info) {
  size_t column = out->printf(kTestMarker " %s/%s", suite, name);
  out->flush();
  double start = get_current_time_seconds();
  TestRunHandle run_handle;
  unit_test(&run_handle);
  double end = get_current_time_seconds();
  double duration = end - start;
  print_time(out, column, duration, &run_handle);
  info->record_run(duration, &run_handle);
}

void MultiTestCaseInfo::run(unit_test_selector_t *selector, tclib::OutStream *out,
    TestRunInfo *info) {
  for (size_t fi = 0; fi < flavorc; fi++) {
    const char *flavor = flavorv[fi];
    if (!selector->matches(suite, name, flavor))
      continue;
    size_t column = out->printf(kTestMarker " %s/%s/%s", suite, name, flavor);
    out->flush();
    double start = get_current_time_seconds();
    TestRunHandle run_handle;
    testv[fi](&run_handle);
    double end = get_current_time_seconds();
    double duration = end - start;
    print_time(out, column, duration, &run_handle);
    info->record_run(duration, &run_handle);
  }
}

bool SingleTestCaseInfo::matches(unit_test_selector_t *selector) {
  return selector->matches(suite, name, NULL);
}

bool MultiTestCaseInfo::matches(unit_test_selector_t *selector) {
  for (size_t i = 0; i < flavorc; i++) {
    if (selector->matches(suite, name, flavorv[i]))
      return true;
  }
  return false;
}

static void parse_test_selector(const char *str, unit_test_selector_t *selector) {
  char *name = NULL;
  char *flavor = NULL;
  char *suite = strdup(str);
  char *name_part = strchr(suite, '/');
  if (name_part != NULL) {
    name_part[0] = 0;
    name = &name_part[1];
    char *flavor_part = strchr(name, '/');
    if (flavor_part != NULL) {
      flavor_part[0] = 0;
      flavor = &flavor_part[1];
    }
  }
  selector->init(suite, name, flavor);
}

TestCaseInfo *TestCaseInfo::chain = NULL;

TestCaseInfo::TestCaseInfo(const char *file, const char *suite, const char *name) {
  this->file = file;
  this->suite = suite;
  this->name = name;
  insert_into_chain();
}

void TestCaseInfo::insert_into_chain() {
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

SingleTestCaseInfo::SingleTestCaseInfo(const char *file, const char *suite,
    const char *name, unit_test_t unit_test)
  : TestCaseInfo(file, suite, name) {
  this->unit_test = unit_test;
}


MultiTestCaseInfo::MultiTestCaseInfo(const char *file, const char *suite, const char *name,
    size_t flavorc, const char **flavorv, unit_test_t *testv)
  : TestCaseInfo(file, suite, name) {
  this->flavorc = flavorc;
  this->flavorv = flavorv;
  this->testv = testv;
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

void TestCaseInfo::run_tests(unit_test_selector_t *selector, tclib::OutStream *out,
    TestRunInfo *info) {
  TestCaseInfo *current = TestCaseInfo::chain;
  while (current != NULL) {
    if (current->matches(selector))
      current->run(selector, out, info);
    current = current->next;
  }
}

void TestCaseInfo::validate_all() {
  TestCaseInfo *current = TestCaseInfo::chain;
  while (current != NULL) {
    current->validate();
    current = current->next;
  }
}

void TestRunInfo::record_run(double duration, TestRunHandle *handle) {
  duration_ += duration;
  if (handle->was_skipped()) {
    skip_count_ += 1;
  } else {
    run_count_ += 1;
  }
}

// Run!
int main(int argc, char *argv[]) {
  // Impose an allocation limit.
  limited_allocator_t limiter;
  limited_allocator_install(&limiter, 100 * 1024 * 1024);
  // Do allocation fingerprinting.
  fingerprinting_allocator_t fingerprinter;
  fingerprinting_allocator_install(&fingerprinter);
  lifetime_t lifetime;
  ASSERT_TRUE(lifetime_begin_default(&lifetime));
  install_crash_handler();
  tclib::OutStream *out = tclib::FileSystem::native()->std_out();

  TestCaseInfo::validate_all();
  TestRunInfo run_info;
  if (argc >= 2) {
    // If there are arguments run the relevant test suites.
    for (int i = 1; i < argc; i++) {
      unit_test_selector_t selector;
      parse_test_selector(argv[i], &selector);
      TestCaseInfo::run_tests(&selector, out, &run_info);
      selector.dispose();
    }
  } else {
    // If there are no arguments just run everything.
    unit_test_selector_t selector;
    TestCaseInfo::run_tests(&selector, out, &run_info);
  }
  lifetime_end_default(&lifetime);
  int default_exit_code = 0;
  if (run_info.total_count() == 0) {
    // If we didn't run any tests print a message and force the run to fail --
    // it's a sign something's off.
    out->printf(kTotalMarker " no tests run\n");
    out->flush();
    default_exit_code = 1;
  } else {
    size_t column = out->printf(kTotalMarker " %i test%s passed", run_info.run_count(),
        run_info.run_count() == 1 ? "" : "s");
    if (run_info.skip_count() > 0)
      column += out->printf(" (%i skipped)", run_info.skip_count());
    TestCaseInfo::print_time(out, column, run_info.duration(), NULL);
  }
  // Return a successful error code only if there were no allocator leaks.
  if (fingerprinting_allocator_uninstall(&fingerprinter)
      && limited_allocator_uninstall(&limiter)) {
    return default_exit_code;
  } else {
    return 1;
  }
}
