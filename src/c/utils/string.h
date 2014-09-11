//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "check.h"
#include "stdc.h"

#ifndef _TCLIB_STRING_H
#define _TCLIB_STRING_H

// A C string with a length.
typedef struct {
  size_t length;
  const char *chars;
} string_t;

// Initializes this string to hold the given characters.
void string_init(string_t *str, const char *chars);

// Returns the length of the given string.
size_t string_length(string_t *str);

// Returns the index'th character in the given string.
char string_char_at(string_t *str, size_t index);

// Write the contents of this string into the given buffer, which must hold
// at least count characters.
void string_copy_to(string_t *str, char *dest, size_t count);

// Returns true iff the two strings are equal.
bool string_equals(string_t *a, string_t *b);

// Returns an integer indicating how a and b relate in lexical ordering. It
// holds that (string_compare(a, b) REL 0) when (a REL b) for a relational
// operator REL.
int string_compare(string_t *a, string_t *b);

// Returns true iff the given string is equal to the given c-string.
bool string_equals_cstr(string_t *a, const char *b);


// A small snippet of a string that can be encoded as a 32-bit integer. Create
// a hint cheaply using the STRING_HINT macro.
typedef struct {
  // The characters of this hint.
  const char value[4];
} string_hint_t;

// Reads the characters from a string hint, storing them in a plain c-string.
void string_hint_to_c_str(const char *hint, char c_str_out[5]);

#endif // _TCLIB_STRING_H
