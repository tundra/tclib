#include "string.h"

void string_init(string_t *str, const char *chars) {
  str->chars = chars;
  str->length = strlen(chars);
}

size_t string_length(string_t *str) {
  return str->length;
}

char string_char_at(string_t *str, size_t index) {
  CHECK_REL("string index out of bounds", index, <, string_length(str));
  return str->chars[index];
}

void string_copy_to(string_t *str, char *dest, size_t count) {
  // The count must be strictly greater than the number of chars because we
  // also need to fit the terminating null character.
  size_t length = string_length(str);
  CHECK_REL("string copy destination too small", length, <, count);
  strncpy(dest, str->chars, length);
  dest[length] = '\0';
}

bool string_equals(string_t *a, string_t *b) {
  size_t length = string_length(a);
  if (length != string_length(b))
    return false;
  for (size_t i = 0; i < length; i++) {
    if (string_char_at(a, i) != string_char_at(b, i))
      return false;
  }
  return true;
}

int string_compare(string_t *a, string_t *b) {
  size_t a_length = string_length(a);
  size_t b_length = string_length(b);
  if (a_length != b_length)
    return a_length - b_length;
  for (size_t i = 0; i < a_length; i++) {
    char a_char = string_char_at(a, i);
    char b_char = string_char_at(b, i);
    if (a_char != b_char)
      return a_char - b_char;
  }
  return 0;
}

bool string_equals_cstr(string_t *a, const char *str) {
  string_t b;
  string_init(&b, str);
  return string_equals(a, &b);
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
