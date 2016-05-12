//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "utils/log.hh"

using namespace tclib;

// Log that counts how many info entries are recorded. We don't count all log
// entries because if the test fails we need those entries to be handled by the
// proper logging framework so the test will fail.
class InfoCounter : public Log {
public:
  InfoCounter() : count_(0), propagate_(false) { }
  virtual fat_bool_t record(log_entry_t *entry);
  size_t count() { return count_; }
  void set_propagate(bool value) { propagate_ = value; }
private:
  size_t count_;
  bool propagate_;
};

fat_bool_t InfoCounter::record(log_entry_t *entry) {
  if (entry->level != llInfo) {
    // Because we're testing the log here we can't rely on the log working so
    // we hard-fail. This is a pain if the test does fail but on the other hand
    // it means we're sure that subtle breakages won't go unnoticed.
    abort_message_t message;
    abort_message_init(&message, lsStderr, __FILE__, __LINE__, 0, "Log test failed");
    abort_call(get_global_abort(), &message);
    return F_FALSE;
  }
  count_++;
  return F_BOOL(!propagate_ || propagate(entry));
}

TEST(log, replace) {
  InfoCounter outer;
  outer.ensure_installed();
  ASSERT_EQ(0, outer.count());
  LOG_INFO("A");
  ASSERT_EQ(1, outer.count());
  {
    InfoCounter inner;
    inner.ensure_installed();
    ASSERT_EQ(1, outer.count());
    ASSERT_EQ(0, inner.count());
    LOG_INFO("B");
    ASSERT_EQ(1, outer.count());
    ASSERT_EQ(1, inner.count());
    LOG_INFO("C");
    ASSERT_EQ(1, outer.count());
    ASSERT_EQ(2, inner.count());
    inner.set_propagate(true);
    LOG_INFO("D");
    ASSERT_EQ(2, outer.count());
    ASSERT_EQ(3, inner.count());
    LOG_INFO("E");
    ASSERT_EQ(3, outer.count());
    ASSERT_EQ(4, inner.count());
    inner.set_propagate(false);
    LOG_INFO("F");
    ASSERT_EQ(3, outer.count());
    ASSERT_EQ(5, inner.count());
    inner.set_propagate(true);
    LOG_INFO("G");
    ASSERT_EQ(4, outer.count());
    ASSERT_EQ(6, inner.count());
  }
  LOG_INFO("H");
  ASSERT_EQ(5, outer.count());
}
