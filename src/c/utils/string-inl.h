//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_STRING_INL_H
#define _TCLIB_STRING_INL_H

#include "utils/string.h"

// Returns the number of characters of the string. The -1 is to get rid of the
// null terminator.
#define __STATIC_STRLEN__(S) ((sizeof(S) / sizeof(char)) - 1)

// Expands to an initializer that can be used to initialize a string_hint_t
// variable.
#define STRING_HINT_INIT(str) {{                                               \
  ((__STATIC_STRLEN__(str) == 0) ? '\0' : str[0]),                             \
  ((__STATIC_STRLEN__(str) <= 1) ? '\0' : str[1]),                             \
  ((__STATIC_STRLEN__(str) <= 3) ? '\0' : str[__STATIC_STRLEN__(str) - 2]),    \
  ((__STATIC_STRLEN__(str) <= 2) ? '\0' : str[__STATIC_STRLEN__(str) - 1]),    \
}}

// Wraps a string_t around a character array and a length.
static utf8_t new_string(const char *chars, size_t size) {
  utf8_t result;
  result.size = size;
  result.chars = chars;
  return result;
}

// Wraps a string_t around a C string.
static utf8_t new_c_string(const char *value) {
  return new_string(value, strlen(value));
}

// Returns the empty utf8-string. This is different from a utf8 that represents
// "" in that "" is actually backed by a 1-element char array containing '\0'
// whereas the empty utf8 contains nothing.
static utf8_t string_empty() {
  return new_string(NULL, 0);
}

// Is the given value the empty utf8 string? Note that this is not the same
// as the empty string; see string_empty().
static bool string_is_empty(utf8_t value) {
  return value.chars == NULL;
}

#endif // _TCLIB_STRING_INL_H
