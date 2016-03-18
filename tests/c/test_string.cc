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

void check_format(const char *expected, const char *fmt, ...) {
  string_buffer_t buf;
  ASSERT_TRUE(string_buffer_init(&buf));

  va_list argp1;
  va_start(argp1, fmt);
  char native[1024];
  vsnprintf(native, 1024, fmt, argp1);
  va_end(argp1);
  ASSERT_C_STREQ(expected, native);

  va_list argp0;
  va_start(argp0, fmt);
  string_buffer_vprintf(&buf, fmt, argp0);
  va_end(argp0);
  ASSERT_C_STREQ(expected, string_buffer_flush(&buf).chars);

  string_buffer_dispose(&buf);
}

TEST(string, string_buffer_format) {
  check_format("(null)", "%s", NULL);
  check_format("%", "%%");
  check_format("3.140", "%.3f", 3.14);
  check_format("3.1400", "%.4f", 3.14);
  check_format("3.141593", "%f", 3.1415926);
  long double ld = 3.1415926;
  check_format("3.141593", "%Lf", ld);
  void *ptr = (void*) 1000;
  check_format(IF_MSVC(IF_32_BIT("", "00000000") "000003E8", "0x3e8"), "%p", ptr);
  check_format("100", "%i", 100);
  check_format("-1", "%i", -1);
  check_format("4294967295", "%u", -1);
  int64_t lli = -1;
  check_format("-1", "%lli", lli);
  check_format("18446744073709551615", "%llu", lli);
}

typedef struct {
  format_handler_o super;
  size_t count;
} test_format_handler_o;

static void format_test_value(format_handler_o *self, format_request_t *request,
    va_list_ref_t argp) {
  test_format_handler_o *handler = (test_format_handler_o*) self;
  char *value = va_arg(VA_LIST_DEREF(argp), char*);
  string_buffer_native_printf(request->buf, "[T %s %i w%i f%i]", value,
      handler->count++, request->width, request->flags);
}

TEST(string, custom_formatter) {
  format_handler_o_vtable_t vtable = { format_test_value };
  test_format_handler_o handler;
  handler.super.header.vtable = &vtable;
  handler.count = 0;
  register_format_handler('t', &handler.super);

  string_buffer_t buf;
  ASSERT_TRUE(string_buffer_init(&buf));
  string_buffer_printf(&buf, "[foo %+8t %#7t %06t bar]", "bah", "hum", "bug");
  ASSERT_C_STREQ("[foo [T bah 0 w8 f2] [T hum 1 w7 f16] [T bug 2 w6 f8] bar]",
      string_buffer_flush(&buf).chars);
  string_buffer_dispose(&buf);

  unregister_format_handler('t');
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

static void assert_conversion(scanf_conversion_t format, scanf_conversion_type_t type,
    char chr, int64_t width) {
  ASSERT_EQ(type, format.type);
  ASSERT_EQ(chr, format.chr);
  ASSERT_EQ(width, format.width);
}

TEST(string, scanf_analyze_format) {
  scanf_conversion_t convs[8];
  ASSERT_EQ(3, string_scanf_analyze_conversions(new_c_string("%i %32s %8x"), convs, 8));
  assert_conversion(convs[0], stSignedInt, 'i', -1);
  assert_conversion(convs[1], stString, 's', 32);
  assert_conversion(convs[2], stSignedInt, 'x', 8);

  ASSERT_EQ(1, string_scanf_analyze_conversions(new_c_string("foo %32s bar"), convs, 8));
  assert_conversion(convs[0], stString, 's', 32);

  ASSERT_EQ(-1, string_scanf_analyze_conversions(new_c_string("foo %s bar"), convs, 8));

  ASSERT_EQ(2, string_scanf_analyze_conversions(new_c_string("%i %*32s %8x"), convs, 8));
  assert_conversion(convs[0], stSignedInt, 'i', -1);
  assert_conversion(convs[1], stSignedInt, 'x', 8);

  ASSERT_EQ(0, string_scanf_analyze_conversions(new_c_string("%% %% %%"), convs, 8));
}

TEST(string, scanf) {
  // Most of the format testing happens in the nunit string.n test.
  scanf_conversion_t convs[8];
  utf8_t fmt = new_c_string("%i %i %i");
  ASSERT_EQ(3, string_scanf_analyze_conversions(fmt, convs, 8));
  int ints[3];
  void *args[3] = {&ints[0], &ints[1], &ints[2]};
  ASSERT_EQ(3, string_scanf(fmt, new_c_string("123 656 532"), convs, 3, args));
  ASSERT_EQ(123, ints[0]);
  ASSERT_EQ(656, ints[1]);
  ASSERT_EQ(532, ints[2]);
}

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

  // 64-bit MSVC gets a little eager with the warnings here -- we want the
  // conditions in STRING_HINT_INIT to evaluate to a statically known value
  // but in this particular case MSVC sees fit to warn that they are:
  //
  //   .\tests\c\test_string.cc(...) : warning C4296: '<=' : expression is always true
  //
  // It's a useful warning in general though so just disable it here.
#ifdef IS_MSVC
#  pragma warning(push)
#  pragma warning(disable: 4296)
#endif
  CHECK_HINT("", "");
#ifdef IS_MSVC
#  pragma warning(pop)
#endif
}

TEST(string, dup_free) {
  utf8_t str = new_c_string("booh!");
  utf8_t str2 = string_default_dup(str);
  ASSERT_TRUE(str.chars != str2.chars);
  ASSERT_STREQ(str, str2);
  string_default_delete(str2);

  utf8_t e2 = string_default_dup(string_empty());
  ASSERT_TRUE(string_is_empty(e2));
  string_default_delete(e2);
}

TEST(string, dup_unterminated) {
  char chars[4] = {'f', 'o', 'o', 'x'};
  utf8_t str = string_default_dup(new_string(chars, 3));
  ASSERT_EQ(0, str.chars[3]);
  string_default_delete(str);
}
