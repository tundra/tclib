//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "string-inl.h"

size_t string_size(utf8_t str) {
  return str.size;
}

uint8_t string_byte_at(utf8_t str, size_t index) {
  CHECK_REL("string index out of bounds", index, <, string_size(str));
  return str.chars[index];
}

utf8_t string_substring(utf8_t str, int64_t from, int64_t to) {
  if (from < 0)
    from = 0;
  size_t size = string_size(str);
  if (to > ((int64_t) size))
    to = size;
  return new_string(str.chars + from, to - from);
}

void string_copy_to(utf8_t str, char *dest, size_t count) {
  // The count must be strictly greater than the number of chars because we
  // also need to fit the terminating null character.
  size_t length = string_size(str);
  CHECK_REL("string copy destination too small", length, <, count);
  strncpy(dest, str.chars, length);
  dest[length] = '\0';
}

bool string_equals(utf8_t a, utf8_t b) {
  size_t size = string_size(a);
  if (size != string_size(b))
    return false;
  for (size_t i = 0; i < size; i++) {
    if (string_byte_at(a, i) != string_byte_at(b, i))
      return false;
  }
  return true;
}

int string_compare(utf8_t a, utf8_t b) {
  size_t a_size = string_size(a);
  size_t b_size = string_size(b);
  if (a_size != b_size)
    return (a_size < b_size) ? -1 : 1;
  for (size_t i = 0; i < a_size; i++) {
    char a_char = string_byte_at(a, i);
    char b_char = string_byte_at(b, i);
    if (a_char != b_char)
      return a_char - b_char;
  }
  return 0;
}

bool string_equals_cstr(utf8_t a, const char *str) {
  return string_equals(a, new_c_string(str));
}

void string_hint_to_c_str(const char *hint, char c_str_out[7]) {
  // The first two characters can always just be copied, even if they're '\0'.
  c_str_out[0] = hint[0];
  c_str_out[1] = hint[1];
  if (hint[3] != '\0') {
    // If the string has a last character we also want to add that.
    if (hint[2] != '\0') {
      // If the string has a one-before-last character we'll have to assume
      // that there might be something in between too so show '..' between the
      // first and last part.
      c_str_out[2] = '.';
      c_str_out[3] = '.';
      c_str_out[4] = hint[2];
      c_str_out[5] = hint[3];
      c_str_out[6] = '\0';
    } else {
      // If there is just a last character the string must have had length 3. So
      // write the third character and terminate.
      c_str_out[2] = hint[3];
      c_str_out[3] = '\0';
    }
  } else {
    // If there is no last character beyond the two first it must have had
    // length 2. Just terminate.
    c_str_out[2] = '\0';
  }
}
