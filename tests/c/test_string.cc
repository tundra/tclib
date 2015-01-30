//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

BEGIN_C_INCLUDES
#include "utils/string-inl.h"
#include "utils/strbuf.h"
END_C_INCLUDES

TEST(string, string_simple) {
  utf8_t str = new_c_string("Hello, World!");
  ASSERT_EQ(13, string_size(str));
  ASSERT_EQ('H', string_byte_at(str, 0));
  ASSERT_EQ('!', string_byte_at(str, 12));
}

TEST(string, string_comparison) {
  // Use char arrays to ensure that the strings are all stored in different
  // parts of memory.
  char c0[4] = {'f', 'o', 'o', '\0'};
  char c1[4] = {'f', 'o', 'o', '\0'};
  char c2[3] = {'f', 'o', '\0'};
  char c3[4] = {'f', 'o', 'f', '\0'};

  utf8_t s0 = new_c_string(c0);
  utf8_t s1 = new_c_string(c1);
  utf8_t s2 = new_c_string(c2);
  utf8_t s3 = new_c_string(c3);

#define ASSERT_STR_COMPARE(A, REL, B)                                          \
  ASSERT_TRUE(string_compare(A, B) REL 0)

  ASSERT_TRUE(string_equals(s0, s0));
  ASSERT_STR_COMPARE(s0, ==, s0);
  ASSERT_TRUE(string_equals(s0, s1));
  ASSERT_STR_COMPARE(s0, ==, s1);
  ASSERT_FALSE(string_equals(s0, s2));
  ASSERT_STR_COMPARE(s0, >, s2);
  ASSERT_FALSE(string_equals(s0, s3));
  ASSERT_STR_COMPARE(s0, >, s3);
  ASSERT_TRUE(string_equals(s1, s0));
  ASSERT_STR_COMPARE(s1, ==, s0);
  ASSERT_TRUE(string_equals(s1, s1));
  ASSERT_STR_COMPARE(s1, ==, s1);
  ASSERT_FALSE(string_equals(s1, s2));
  ASSERT_STR_COMPARE(s1, >, s2);
  ASSERT_FALSE(string_equals(s1, s3));
  ASSERT_STR_COMPARE(s1, >, s3);
  ASSERT_FALSE(string_equals(s2, s0));
  ASSERT_STR_COMPARE(s2, <, s0);
  ASSERT_FALSE(string_equals(s2, s1));
  ASSERT_STR_COMPARE(s2, <, s1);
  ASSERT_TRUE(string_equals(s2, s2));
  ASSERT_STR_COMPARE(s2, ==, s2);
  ASSERT_FALSE(string_equals(s2, s3));
  ASSERT_STR_COMPARE(s2, <, s3);
  ASSERT_FALSE(string_equals(s3, s0));
  ASSERT_STR_COMPARE(s3, <, s0);
  ASSERT_FALSE(string_equals(s3, s1));
  ASSERT_STR_COMPARE(s3, <, s1);
  ASSERT_FALSE(string_equals(s3, s2));
  ASSERT_STR_COMPARE(s3, >, s2);
  ASSERT_TRUE(string_equals(s3, s3));
  ASSERT_STR_COMPARE(s3, ==, s3);

#undef ASSERT_STR_COMPARE
}

TEST(string, string_buffer_simple) {
  string_buffer_t buf;
  string_buffer_init(&buf);

  string_buffer_printf(&buf, "[%s: %i]", "test", 8);
  utf8_t str = string_buffer_flush(&buf);
  utf8_t expected = new_c_string("[test: 8]");
  ASSERT_STREQ(expected, str);

  string_buffer_dispose(&buf);
}

TEST(string, string_buffer_format) {
  string_buffer_t buf;
  string_buffer_init(&buf);

  string_buffer_printf(&buf, "[S %s] ", NULL);
  string_buffer_printf(&buf, "[P %%]");
  utf8_t str = string_buffer_flush(&buf);
  utf8_t expected = new_c_string("[S (NULL)] [P %]");
  ASSERT_STREQ(expected, str);

  string_buffer_dispose(&buf);
}

TEST(string, string_buffer_concat) {
  string_buffer_t buf;
  string_buffer_init(&buf);

  string_buffer_printf(&buf, "foo");
  string_buffer_printf(&buf, "bar");
  string_buffer_printf(&buf, "baz");
  utf8_t str = string_buffer_flush(&buf);
  utf8_t expected = new_c_string("foobarbaz");
  ASSERT_STREQ(expected, str);

  string_buffer_dispose(&buf);
}

TEST(string, string_buffer_long) {
  string_buffer_t buf;
  string_buffer_init(&buf);

  // Cons up a really long string.
  for (size_t i = 0; i < 1024; i++)
    string_buffer_printf(&buf, "0123456789");
  // Check that it's correct.
  utf8_t str = string_buffer_flush(&buf);
  ASSERT_EQ(10240, string_size(str));
  for (size_t i = 0; i < 10240; i++) {
    ASSERT_EQ((char) ('0' + (i % 10)), string_byte_at(str, i));
  }

  string_buffer_dispose(&buf);
}

#define CHECK_PRINTF(expected, format, ...) do {                               \
  string_buffer_t buf;                                                         \
  string_buffer_init(&buf);                                                    \
  string_buffer_printf(&buf, format, __VA_ARGS__);                             \
  string_t found;                                                              \
  string_buffer_flush(&buf, &found);                                           \
  string_t str = new_string(expected);                                                \
  ASSERT_STREQ(&str, &found);                                                  \
  string_buffer_dispose(&buf);                                                 \
} while (false)

#define CHECK_HINT(HINT, EXPECTED) do {                                        \
  string_hint_t hint = STRING_HINT_INIT(HINT);                                 \
  char hint_str[7];                                                            \
  string_hint_to_c_str(hint.value, hint_str);                                  \
  ASSERT_C_STREQ(EXPECTED, hint_str);                                          \
} while (false)

TEST(string, string_hint) {
  ASSERT_EQ(sizeof(uint32_t), sizeof(string_hint_t));

  CHECK_HINT("abcdef", "ab..ef");
  CHECK_HINT("abcde", "ab..de");
  CHECK_HINT("abcd", "ab..cd");
  CHECK_HINT("abc", "abc");
  CHECK_HINT("ab", "ab");
  CHECK_HINT("a", "a");
  CHECK_HINT("", "");
}

