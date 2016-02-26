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
  ASSERT_EQ(23, fat_bool_line(c));
  ASSERT_EQ(kOwnFileId, fat_bool_file(c));
  fat_bool_t d = F_FALSE;
  ASSERT_EQ(29, fat_bool_line(d));
  ASSERT_EQ(fat_bool_file(c), fat_bool_file(d));
}

TEST(fatbool, accessors) {
  fat_bool_t fb1 = fat_bool_false(523, 432);
  ASSERT_EQ(523, fat_bool_file(fb1));
  ASSERT_EQ(432, fat_bool_line(fb1));
  int32_t code = fat_bool_code(fb1);
  fat_bool_t fb2 = fat_bool_new(code);
  ASSERT_EQ(523, fat_bool_file(fb2));
  ASSERT_EQ(432, fat_bool_line(fb2));
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
  ASSERT_EQ(kOwnFileId, fat_bool_file(fb));
  ASSERT_EQ(45, fat_bool_line(fb));
}
