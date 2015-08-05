//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_STRING_H
#define _TCLIB_STRING_H

#include "c/stdc.h"
#include "utils/check.h"

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

// Returns a string that represents the substring of the input starting from
// $from and up to but not including $to. The boundaries are clamped to the
// input string's boundaries.
utf8_t string_substring(utf8_t str, int64_t from, int64_t to);

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

// The type of value extracted by scanf for a particular conversion character.
typedef enum {
  stUnknown,
  stSignedInt,
  stString,
} scanf_conversion_type_t;

// Description of a scanf conversion specifier.
typedef struct {
  scanf_conversion_type_t type;
  int64_t width;
  char chr;
} scanf_conversion_t;

// Scan the given scanf format string and store information about the conversion
// specifiers in the given conversion vector that is at least convc long.
// Returns the number of conversions seen unless the format is invalid in which
// case -1 is returned.
int64_t string_scanf_analyze_conversions(utf8_t format, scanf_conversion_t *convv,
    size_t convc);

// Scans the given input using the given format string whose format specifiers
// are described by the given format vector. Stores outputs in the pointer
// vector. Returns the number of formats matched or -1 if matching fails.
int64_t string_scanf(utf8_t format, utf8_t input, scanf_conversion_t *convv,
    size_t convc, void **ptrs);

// A small snippet of a string that can be encoded as a 32-bit integer. Create
// a hint cheaply using the STRING_HINT macro.
typedef struct {
  // The characters of this hint.
  const char value[4];
} string_hint_t;

// Reads the characters from a string hint, storing them in a plain c-string.
void string_hint_to_c_str(const char *hint, char c_str_out[5]);

#endif // _TCLIB_STRING_H
