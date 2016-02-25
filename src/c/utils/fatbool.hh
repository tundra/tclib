//- Copyright 2016 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _UTILS_FATBOOL_H
#define _UTILS_FATBOOL_H

#ifndef FILE_ID
#  error "FILE_ID not defined; use mkmk's c.get_settings().set_pervasive('gen_fileid', True)."
#endif

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "utils/opaque.h"
END_C_INCLUDES

// Encodes a file and line id as a fat-bool code.
#define __JOIN_IDS__(FID, LID) ((static_cast<uint32_t>(FID) << 16) | (static_cast<uint32_t>(LID) + 1))

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
  explicit fat_bool_t(uint32_t code) : code_(code) { }

  // Initializes a false fat-bool with the given file id and line.
  explicit fat_bool_t(uint32_t file_id, int32_t line) : code_(__JOIN_IDS__(file_id, line)) { }

  // Returns the negative value of this fat-bool as a plain bool.
  bool operator!() { return code_ != 0; }

  // Converts this fat-bool to a plain bool.
  operator bool() { return code_ == 0; }

  // Returns the id of the file where this fat-bool originated. If you don't
  // know where that is use the --dump-file-ids flag to mkmk to generate a list
  // of mappings.
  uint32_t file_id() { return code_ >> 16; }

  // Returns the line where this fat-bool originated.
  uint32_t line() { return (code_ & 0xFFFF) - 1; }

  // Returns the raw code of this fat-bool.
  uint32_t code() { return code_; }

private:
  uint32_t code_;
};

// Expands to the true fat-bool value.
#define F_TRUE fat_bool_t()

// Expands to a false fat-bool value that captures the location of the
// expression.
#define F_FALSE fat_bool_t(__JOIN_IDS__(FILE_ID, __LINE__))

// Returns a fat-bool representing the same boolean value, capturing the
// location if the value is false.
#define F_BOOL(v) ((v) ? F_TRUE : F_FALSE)

// Evaluate the given expression, if it returns a fat false value then return
// that value from the current context.
#define F_TRY(EXPR) do {                                                       \
  fat_bool_t __value__ = (EXPR);                                               \
  if (!__value__)                                                              \
    return __value__;                                                          \
} while (false)

// Returns an opaque whose o2b yields the given boolean value.
static always_inline opaque_t f2o(fat_bool_t value) {
  return u2o(value.code());
}

// Returns the boolean value of an opaque that was created using b2o.
static always_inline fat_bool_t o2f(opaque_t opaque) {
  return fat_bool_t(static_cast<int32_t>(o2u(opaque)));
}

#endif // _UTILS_FATBOOL_H
