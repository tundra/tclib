//- Copyright 2016 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _UTILS_FATBOOL_H
#define _UTILS_FATBOOL_H

#ifndef FILE_ID
#  error "FILE_ID not defined; use mkmk's c.get_settings().set_pervasive('gen_fileid', True)."
#endif

#include "c/stdc.h"

#include "utils/opaque.h"

// The default format string to use when printing a fat-bool's file and line.
#define kFatBoolFileLine "0x%04x:%i"

// Encodes a file and line id as a fat-bool code.
#define __JOIN_IDS__(FID, LID) ((((uint32_t) (FID)) << 16) | (((uint32_t) (LID)) + 1))

#define FAT_BOOL_TO_BOOL(B) ((B).code == 0)

// A fat bool is similar to a boolean in that it's a small integer, really, but
// if it's false it encodes a file and line where it originated. Each file has
// an id based on the basename of the toplevel file (so includes are not
// considered) and you can use mkmk to generate a mapping from ids to files.
// You generally don't want to construct these explicitly, rather use the macros
// below.
typedef struct fat_bool_t {
  uint32_t code;
  // Converts this fat-bool to a plain bool.
  ONLY_CPP(always_inline operator bool() { return FAT_BOOL_TO_BOOL(*this); })
  // Returns the negative value of this fat-bool as a plain bool.
  ONLY_CPP(always_inline bool operator!() { return !FAT_BOOL_TO_BOOL(*this); })
  // Eager-and of this and the given value.
  ONLY_CPP(always_inline fat_bool_t operator&(fat_bool_t that);)
  // Eager-or of this and the given value.
  ONLY_CPP(always_inline fat_bool_t operator|(fat_bool_t that);)
} fat_bool_t;

// Eager-and of this and the given value.
static always_inline fat_bool_t fat_bool_and(fat_bool_t a, fat_bool_t b) {
  return FAT_BOOL_TO_BOOL(a) ? b : a;
}

// Eager-or of this and the given value.
static always_inline fat_bool_t fat_bool_or(fat_bool_t a, fat_bool_t b) {
  return FAT_BOOL_TO_BOOL(a) ? a : b;
}

ONLY_CPP(fat_bool_t fat_bool_t::operator&(fat_bool_t that) { return fat_bool_and(*this, that); })

ONLY_CPP(fat_bool_t fat_bool_t::operator|(fat_bool_t that) { return fat_bool_or(*this, that); })

// Returns a fat-bool value with the given code.
static always_inline fat_bool_t fat_bool_new(uint32_t code) {
  fat_bool_t result = {code};
  return result;
}

// Returns a false fat-bool with the given file and line.
static always_inline fat_bool_t fat_bool_false(uint32_t file, uint32_t line) {
  fat_bool_t result = {__JOIN_IDS__(file, line)};
  return result;
}

// Returns the line where this fat-bool originated.
static always_inline uint32_t fat_bool_line(fat_bool_t value) {
  return (value.code & 0xFFFF) - 1;
}

// Returns the id of the file where this fat-bool originated. If you don't
// know where that is use the --dump-file-ids flag to mkmk to generate a list
// of mappings.
static always_inline uint32_t fat_bool_file(fat_bool_t value) {
  return value.code >> 16;
}

// Returns the raw code of this fat-bool. Use fat_bool_new to get back a
// fat-bool with the same value as the argument.
static always_inline uint32_t fat_bool_code(fat_bool_t value) {
  return value.code;
}

// Setting this to 1 causes F_TRY to log a line each time it propagates false
// which can be really useful when chasing down failures. Must be 0 when
// submitting though.
#define F_TRACE_FAILURES 0

// Expands to the true fat-bool value.
#define F_TRUE fat_bool_new(0)

// Expands to a false fat-bool value that captures the location of the
// expression.
#define F_FALSE fat_bool_false(FILE_ID, __LINE__)

// Returns a fat-bool representing the same boolean value, capturing the
// location if the value is false.
#define F_BOOL(v) fat_bool_new((v) ? 0 : __JOIN_IDS__(FILE_ID, __LINE__))

// Report a false fat-bool through the logging infrastructure.
void fat_bool_log(const char *file, int line, fat_bool_t error);

#define F_LOG_FALSE(EXPR) do {                                                 \
  fat_bool_t __value__ = (EXPR);                                               \
  if (!FAT_BOOL_TO_BOOL(__value__))                                            \
    fat_bool_log(__FILE__, __LINE__, __value__);                               \
} while (false)

#if F_TRACE_FAILURES
#  ifndef ALLOW_DEVUTILS
#    error "Tracing fatbool failures not allowed"
#  endif
#  define F_TRY(EXPR) do {                                                     \
  fat_bool_t __value__ = (EXPR);                                               \
  if (!FAT_BOOL_TO_BOOL(__value__)) {                                          \
    fat_bool_log(__FILE__, __LINE__, __value__);                               \
    return __value__;                                                          \
  }                                                                            \
} while (false)
#else
// Evaluate the given expression, if it returns a fat false value then return
// that value from the current context.
#  define F_TRY(EXPR) do {                                                     \
  fat_bool_t __value__ = (EXPR);                                               \
  if (!FAT_BOOL_TO_BOOL(__value__))                                            \
    return __value__;                                                          \
} while (false)
#endif

// Used together with F_YRT, like so:
//
//   F_TRY_EXCEPT(expr);
//     <except block>
//   F_YRT();
//
// This works as follows: first expr is executed, then if it yields a false
// fat-bool the except block is executed and then the result of expr is
// returned. If it yields the true value the except block is skipped.
//
#define F_TRY_EXCEPT(EXPR) do {                                                \
  fat_bool_t __value__ = (EXPR);                                               \
  if (!__value__) {                                                            \
    do { } while (false)

#define F_YRT()                                                                \
    return __value__;                                                          \
  }                                                                            \
} while (false)

// Returns an opaque whose o2b yields the given boolean value.
static always_inline opaque_t f2o(fat_bool_t value) {
  return u2o(value.code);
}

// Returns the boolean value of an opaque that was created using b2o.
static always_inline fat_bool_t o2f(opaque_t opaque) {
  return fat_bool_new((uint32_t) o2u(opaque));
}

#endif // _UTILS_FATBOOL_H
