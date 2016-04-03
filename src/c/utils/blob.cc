//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "utils/blob.hh"

#include "io/stream.hh"
#include "utils/log.hh"

using namespace tclib;

template <typename U, typename S>
void dump_blob(Blob blob, OutStream *out, Blob::DumpStyle style, const char *hexfmt,
    const char *decfmt) {
  size_t blocksize = sizeof(U);
  size_t words_per_line = style.bytes_per_line / blocksize;
  U *words = static_cast<U*>(blob.start());
  byte_t *bytes = static_cast<byte_t*>(blob.start());
  bool after_newline = true;
  for (size_t ib = 0; ib < blob.size(); ib += blocksize) {
    size_t iw = (ib / blocksize);
    // Print newlines and spaces as appropriate.
    if ((ib > 0) && ((iw % words_per_line) == 0)) {
      out->printf("\n");
      after_newline = true;
    }
    if (after_newline) {
      out->printf("%s", style.line_prefix);
      after_newline = false;
    } else {
      out->printf(" ");
    }
    // Print the next word.
    U uword;
    if (ib + blocksize <= blob.size()) {
      uword = words[iw];
    } else {
      uword = 0;
      for (size_t ic = ib; ic < blob.size(); ic++)
        uword = static_cast<U>((uword << 8) | bytes[ic]);
    }
    S sword = static_cast<S>(uword);
    if (style.show_hex)
      out->printf(hexfmt, uword);
    if (style.show_base_10)
      out->printf(decfmt, sword);
    if (style.show_ascii) {
      out->printf("[");
      for (size_t ic = ib; (ic < (ib + blocksize)) && (ic < blob.size()); ic++) {
        byte_t c = bytes[ic];
        if (c < 0x20) {
          out->printf("\\%x", c);
        } else {
          out->printf("%c", c);
        }
      }
      out->printf("]");
    }
  }
}

void Blob::dump(OutStream *out, DumpStyle style) {
  switch (style.word_size) {
    case 1:
      dump_blob<uint8_t, int8_t>(*this, out, style, "%02x", "(%3i)");
      return;
    case 2:
      dump_blob<uint16_t, int16_t>(*this, out, style, "%04x", "(%5i)");
      return;
    case 4:
      dump_blob<uint32_t, int32_t>(*this, out, style, "%08x", "(%10i)");
      return;
    case 8:
      dump_blob<uint64_t, int64_t>(*this, out, style, "%016" PRIx64, "(%20" PRIi64 ")");
      return;
    default:
      ERROR("Invalid word size %i", style.word_size);
      return;
  }
}

bool blob_equals(blob_t a, blob_t b) {
  if (a.size != b.size)
    return false;
  // If we ever need to do this for something performance sensitive probably
  // change it to use strncmp or equivalent.
  for (size_t i = 0; i < a.size; i++) {
    if (static_cast<uint8_t*>(a.start)[i] != static_cast<uint8_t*>(b.start)[i])
      return false;
  }
  return true;
}
