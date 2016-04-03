//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_UTILS_BLOB_HH
#define _TCLIB_UTILS_BLOB_HH

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "utils/blob.h"
END_C_INCLUDES

namespace tclib {

class OutStream;

class Blob : public blob_t {
public:
  // Creates a new empty blob.
  Blob() { *static_cast<blob_t*>(this) = blob_empty(); }

  Blob(blob_t that) { *static_cast<blob_t*>(this) = that; }

  // Creates a new blob with the given contents.
  Blob(const void *start, size_t size) { *static_cast<blob_t*>(this) = blob_new(const_cast<void*>(start), size); }

  // Returns the address of the beginning of this block.
  void *start() { return blob_t::start; }

  // Returns the address immediately past the end of this block.
  void *end() { return static_cast<byte_t*>(start()) + size(); }

  // Fills all this blob with the given value.
  void fill(byte_t value) { blob_fill(*this, value); }

  // The size in bytes of this block.
  size_t size() { return blob_t::size; }

  // Is this the empty blob?
  bool is_empty() { return blob_is_empty(*this); }

  // Returns true if both blobs have the same contents.
  bool operator==(const Blob &that) const  { return blob_equals(*this, that); }

  // Configuration of how to dump a blob textually.
  class DumpStyle {
  public:
    DumpStyle()
      : word_size(4)
      , bytes_per_line(16)
      , show_hex(true)
      , show_base_10(false)
      , show_ascii(false)
      , line_prefix("") { }
    size_t word_size;
    size_t bytes_per_line;
    bool show_hex;
    bool show_base_10;
    bool show_ascii;
    const char *line_prefix;
  };

  // Write this blob to the given output stream using the given style.
  void dump(tclib::OutStream *out, DumpStyle style = DumpStyle());

};

} // namespace tclib

#endif // _TCLIB_UTILS_BLOB_HH
