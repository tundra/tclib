//- Copyright 2016 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _UTILS_FATBOOL_H
#define _UTILS_FATBOOL_H

#ifndef FILE_ID
#  error "FILE_ID not defined; use mkmk's c.get_settings().set_pervasive('gen_fileid', True)."
#endif

#include "c/stdc.h"

// Encodes a file and line id as a fat-bool code.
#define __JOIN_IDS__(FID, LID) ((FID << 16) | (LID + 1))

// A fat bool is similar to a boolean in that it's a small integer, really, but
// if it's false it encodes a file and line where it originated. Each file has
// an id based on the basename of the toplevel file (so includes are not
// considered) and you can use mkmk to generate a mapping from ids to files.
// You generally don't want to construct these explicitly, rather use the macros
// below.
class fat_bool_t {
public:
  // Initializes the true fat-bool.
  explicit fat_bool_t() : code_(0) { }

  // Initializes a fat-bool with the given encoded value.
  explicit fat_bool_t(int32_t code) : code_(code) { }

  // Initializes a false fat-bool with the given file id and line.
  explicit fat_bool_t(int32_t file_id, int32_t line) : code_(__JOIN_IDS__(file_id, line)) { }

  // Returns the negative value of this fat-bool as a plain bool.
  bool operator!() { return code_ != 0; }

  // Converts this fat-bool to a plain bool.
  operator bool() { return code_ == 0; }

  // Returns the id of the file where this fat-bool originated. If you don't
  // know where that is use the --dump-file-ids flag to mkmk to generate a list
  // of mappings.
  int32_t file_id() { return code_ >> 16; }

  // Returns the line where this fat-bool originated.
  int32_t line() { return (code_ & 0xFFFF) - 1; }

  // Returns the raw code of this fat-bool.
  int32_t code() { return code_; }

private:
  int32_t code_;
};

// Expands to the true fat-bool value.
#define F_TRUE fat_bool_t()

// Expands to a false fat-bool value that captures the location of the
// expression.
#define F_FALSE fat_bool_t(__JOIN_IDS__(FILE_ID, __LINE__))

// Evaluate the given expression, if it returns a fat false value then return
// that value from the current context.
#define F_TRY(EXPR) do {                                                       \
  fat_bool_t __value__ = (EXPR);                                               \
  if (!__value__)                                                              \
    return __value__;                                                          \
} while (false)

#endif // _UTILS_FATBOOL_H
