//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "utils/alloc.h"
#include "utils/string-inl.h"

size_t string_size(utf8_t str) {
  return str.size;
}

uint8_t string_byte_at(utf8_t str, size_t index) {
  CHECK_REL("string index out of bounds", index, <, string_size(str));
  return (uint8_t) str.chars[index];
}

utf8_t string_substring(utf8_t str, int64_t from, int64_t to) {
  if (from < 0)
    from = 0;
  size_t size = string_size(str);
  if (to > ((int64_t) size))
    to = (int64_t) size;
  return new_string(str.chars + from, (size_t) (to - from));
}

void string_copy_to(utf8_t str, char *dest, size_t count) {
  // The count must be strictly greater than the number of chars because we
  // also need to fit the terminating null character.
  size_t length = string_size(str);
  CHECK_REL("string copy destination too small", length, <, count);
  strncpy(dest, str.chars, length);
  dest[length] = '\0';
}

utf8_t string_default_dup(utf8_t str) {
  if (string_is_empty(str))
    return str;
  size_t size = sizeof(char) * (str.size + 1);
  blob_t buf = allocator_default_malloc(size);
  blob_copy_to(blob_new((void*) str.chars, size), buf);
  return new_string((char*) buf.start, str.size);
}

void string_default_delete(utf8_t str) {
  if (string_is_empty(str))
    return;
  size_t size = sizeof(char) * (str.size + 1);
  allocator_default_free(blob_new((void*) str.chars, size));
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
    uint8_t a_char = string_byte_at(a, i);
    uint8_t b_char = string_byte_at(b, i);
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

static scanf_conversion_type_t get_scanf_specifier_type(char c) {
  switch (c) {
    case 'd': case 'i': case 'x':
      return stSignedInt;
    case 's': case 'c': case '[':
      return stString;
    default:
      return stUnknown;
  }
}

int64_t string_scanf_analyze_conversions(utf8_t format, scanf_conversion_t *convv,
    size_t convc) {
  // This is kind of excruciating but we need at least a minimal string pattern
  // matching capability and this is less painful than full regular expressions.
  // If vsscanf existed on windows this would also be easier but alas it is not.
  //
  // Making this worse is that if we get this wrong there is the potential for
  // security issues so it's really important that we follow the spec, even for
  // input that's not likely to ever appear. Based on K&R B1.3, formatted input.
  size_t index = 0;
  for (const char *p = format.chars; *p != '\0'; p++) {
    if (*p != '%')
      continue;
    p++;
    if (*p == '%')
      // A literal '%'.
      continue;
    // The optional suppression character.
    bool suppress = false;
    if (*p == '*') {
      suppress = true;
      p++;
    }
    // The optional width specifier.
    int64_t width = -1;
    while (('0' <= *p) && (*p <= '9')) {
      if (width == -1)
        // Only initialize to 0 if there is an actual width.
        width = 0;
      width = (10 * width) + (*p - '0');
      if (width > 0xFFFFFF)
        // Limit the width artificially since overflowing it would be Bad(TM).
        return -1;
      p++;
    }
    // There is no explicit facility for catching the length prefixes because
    // they'll get caught by the type specifier check below.
    if (!*p)
      // If we're at the end of the input it was invalid.
      return -1;
    char c = *p;
    if (c == '[') {
      // If this is a character class then skip to the end to avoid interpreting
      // nested format specifiers.
      p++;
      bool swallow_bracket = true;
      for (size_t i = 0; *p && (*p != ']' || swallow_bracket); i++) {
        // Skip across all characters until the next ] except if it's the very
        // first character in which case it's okay or follows an initial '^'.
        swallow_bracket = (i == 0 && *p == '^');
        p++;
      }
      if (!*p)
        // If we made it all the way to the end without seeing a close bracket
        // the input is invalid.
        return -1;
    }
    scanf_conversion_type_t type = get_scanf_specifier_type(c);
    if (type == stUnknown)
      return -1;
    if (index == convc || (width == -1 && type == stString))
      return -1;
    if (suppress)
      // Go all the way through the checks before using the suppression since
      // that way suppression is orthogonal to which specifiers are valid.
      continue;
    scanf_conversion_t next = {type, width, c};
    convv[index] = next;
    index++;
  }
  return (int64_t) index;
}

int64_t string_scanf(utf8_t format, utf8_t input, scanf_conversion_t *convv,
    size_t convc, void **ptrs) {
  switch (convc) {
    case 0:
      // The NULL makes warnings go away.
      return sscanf(input.chars, format.chars, NULL);
    case 1:
      return sscanf(input.chars, format.chars, ptrs[0]);
    case 2:
      return sscanf(input.chars, format.chars, ptrs[0], ptrs[1]);
    case 3:
      return sscanf(input.chars, format.chars, ptrs[0], ptrs[1], ptrs[2]);
    case 4:
      return sscanf(input.chars, format.chars, ptrs[0], ptrs[1], ptrs[2], ptrs[3]);
    case 5:
      return sscanf(input.chars, format.chars, ptrs[0], ptrs[1], ptrs[2], ptrs[3],
          ptrs[4]);
    case 6:
      return sscanf(input.chars, format.chars, ptrs[0], ptrs[1], ptrs[2], ptrs[3],
          ptrs[4], ptrs[5]);
    case 7:
      return sscanf(input.chars, format.chars, ptrs[0], ptrs[1], ptrs[2], ptrs[3],
          ptrs[4], ptrs[5], ptrs[6]);
    default:
      return -1;
  }
}
