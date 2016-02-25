//- Copyright 2016 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "utils/fatbool.hh"

BEGIN_C_INCLUDES
#include "c/stdc-inl.h"
END_C_INCLUDES

#include <math.h>

// Beware: this test uses line numbers within this file so it's super fragile
// if you insert or remove lines. So just remember to update all the line number
// tests.

// This file's id hardcoded.
#define kOwnFileId 0x3b1a

TEST(fatbool, simple) {
  fat_bool_t b = F_TRUE;
  ASSERT_FALSE(!b);
  fat_bool_t c = F_FALSE;
  ASSERT_TRUE(!c);
  if (c)
    ASSERT_TRUE(false);
  ASSERT_EQ(23, c.line());
  ASSERT_EQ(kOwnFileId, c.file_id());
  fat_bool_t d = F_FALSE;
  ASSERT_EQ(29, d.line());
  ASSERT_EQ(c.file_id(), d.file_id());
}

TEST(fatbool, accessors) {
  fat_bool_t fb1(523, 432);
  ASSERT_EQ(523, fb1.file_id());
  ASSERT_EQ(432, fb1.line());
  int32_t code = fb1.code();
  fat_bool_t fb2(code);
  ASSERT_EQ(523, fb2.file_id());
  ASSERT_EQ(432, fb2.line());
}

static fat_bool_t fail2() {
  return F_FALSE;
}

static fat_bool_t fail1() {
  F_TRY(fail2());
  return F_TRUE;
}

static fat_bool_t fail0() {
  F_TRY(fail1());
  return F_TRUE;
}

TEST(fatbool, try_false) {
  fat_bool_t fb = fail0();
  ASSERT_EQ(kOwnFileId, fb.file_id());
  ASSERT_EQ(45, fb.line());
}
