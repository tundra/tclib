//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "check.h"
#include "stdc.h"

#ifndef _TCLIB_STRING_H
#define _TCLIB_STRING_H

// A C string with a length.
typedef struct {
  size_t size;
  const char *chars;
} utf8_t;

// Returns the number of bytes in the given utf8 string.
size_t string_size(utf8_t str);

// Returns the index'th byte in the given string. Note that because utf8 is a
// multibyte encoding this does not correspond to characters, that's only the
// case if you know the input is an ascii string.
uint8_t string_byte_at(utf8_t str, size_t index);

// Write the contents of this string into the given buffer, which must hold
// at least count characters.
void string_copy_to(utf8_t str, char *dest, size_t count);

// Returns true iff the two strings are equal.
bool string_equals(utf8_t a, utf8_t b);

// Returns an integer indicating how a and b relate in lexical ordering. It
// holds that (string_compare(a, b) REL 0) when (a REL b) for a relational
// operator REL.
int string_compare(utf8_t a, utf8_t b);

// Returns true iff the given string is equal to the given c-string.
bool string_equals_cstr(utf8_t a, const char *b);

// A small snippet of a string that can be encoded as a 32-bit integer. Create
// a hint cheaply using the STRING_HINT macro.
typedef struct {
  // The characters of this hint.
  const char value[4];
} string_hint_t;

// Reads the characters from a string hint, storing them in a plain c-string.
void string_hint_to_c_str(const char *hint, char c_str_out[5]);

#endif // _TCLIB_STRING_H
