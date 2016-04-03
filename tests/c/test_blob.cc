//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"

#include "utils/blob.hh"

BEGIN_C_INCLUDES
#include "utils/string-inl.h"
END_C_INCLUDES

using namespace tclib;

TEST(blob, constr) {
  int i = 0;
  Blob b(&i, 100);
  ASSERT_PTREQ(&i, b.start());
  ASSERT_EQ(100, b.size());
  ASSERT_PTREQ(reinterpret_cast<byte_t*>(&i) + 100, b.end());
}

static void check_dump(const char *expected, Blob blob, Blob::DumpStyle style) {
  StringOutStream stream;
  blob.dump(&stream, style);
  ASSERT_C_STREQ(expected, stream.flush_string().chars);
}

TEST(blob, dump) {
  uint32_t d32[6] = {0xFABACEAE, 0x00FACADE, 0x00DECADE, 0x0BADCAFE, 0xFFFFFFFF, 0x7FFFFFFF};
  tclib::Blob b32(d32, sizeof(uint32_t) * 6);
  Blob::DumpStyle style;
  style.bytes_per_line = b32.size();

  style.word_size = 8;
  check_dump("00facadefabaceae 0badcafe00decade 7fffffffffffffff", b32, style);
  style.word_size = 4;
  check_dump("fabaceae 00facade 00decade 0badcafe ffffffff 7fffffff", b32, style);
  style.word_size = 2;
  check_dump("ceae faba cade 00fa cade 00de cafe 0bad ffff ffff ffff 7fff", b32, style);
  style.word_size = 1;
  check_dump("ae ce ba fa de ca fa 00 de ca de 00 fe ca ad 0b ff ff ff ff ff ff ff 7f", b32, style);

  style.show_hex = false;
  style.show_base_10 = true;

  style.word_size = 8;
  check_dump("(   70591803215761070) (  841551897673255646) ( 9223372036854775807)", b32, style);
  style.word_size = 4;
  check_dump("( -88420690) (  16435934) (  14600926) ( 195939070) (        -1) (2147483647)", b32, style);
  style.word_size = 2;
  check_dump("(-12626) (-1350) (-13602) (  250) (-13602) (  222) (-13570) ( 2989) (   -1) (   -1) (   -1) (32767)", b32, style);
  style.word_size = 1;
  check_dump("(-82) (-50) (-70) ( -6) (-34) (-54) ( -6) (  0) (-34) (-54) (-34) (  0) ( -2) (-54) (-83) ( 11) ( -1) ( -1) ( -1) ( -1) ( -1) ( -1) ( -1) (127)", b32, style);

  style.show_hex = true;
  style.word_size = 4;
  check_dump("fabaceae( -88420690) 00facade(  16435934) 00decade(  14600926) 0badcafe( 195939070) ffffffff(        -1) 7fffffff(2147483647)", b32, style);

  char da[24] = "Lorem ipsum dolor sit..";
  tclib::Blob ba(da, 24);

  style.show_base_10 = false;
  check_dump("65726f4c 7069206d 206d7573 6f6c6f64 69732072 002e2e74", ba, style);

  style.show_ascii = true;
  check_dump("65726f4c[Lore] 7069206d[m ip] 206d7573[sum ] 6f6c6f64[dolo] 69732072[r si] 002e2e74[t..\\0]", ba, style);

  tclib::Blob bas(da, 21);
  style.word_size = 8;
  check_dump("7069206d65726f4c[Lorem ip] 6f6c6f64206d7573[sum dolo] 0000007220736974[r sit]", bas, style);
  style.word_size = 4;
  check_dump("65726f4c[Lore] 7069206d[m ip] 206d7573[sum ] 6f6c6f64[dolo] 69732072[r si] 00000074[t]", bas, style);
  style.word_size = 2;
  check_dump("6f4c[Lo] 6572[re] 206d[m ] 7069[ip] 7573[su] 206d[m ] 6f64[do] 6f6c[lo] 2072[r ] 6973[si] 0074[t]", bas, style);

  style.show_base_10 = true;
  style.bytes_per_line = 8;
  style.word_size = 1;
  check_dump(
      "4c( 76)[L] 6f(111)[o] 72(114)[r] 65(101)[e] 6d(109)[m] 20( 32)[ ] 69(105)[i] 70(112)[p]\n"
      "73(115)[s] 75(117)[u] 6d(109)[m] 20( 32)[ ] 64(100)[d] 6f(111)[o] 6c(108)[l] 6f(111)[o]\n"
      "72(114)[r] 20( 32)[ ] 73(115)[s] 69(105)[i] 74(116)[t]", bas, style);

  style.show_base_10 = false;
  style.word_size = 4;
  style.line_prefix = "->- ";
  check_dump(
      "->- 65726f4c[Lore] 7069206d[m ip]\n"
      "->- 206d7573[sum ] 6f6c6f64[dolo]\n"
      "->- 69732072[r si] 00000074[t]", bas, style);
}
