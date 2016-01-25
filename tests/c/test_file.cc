//- Copyright 2016 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "test/unittest.hh"
#include "io/iop.hh"
#include "io/file.hh"

BEGIN_C_INCLUDES
#include "utils/string-inl.h"
END_C_INCLUDES

using namespace tclib;

TEST(file, tempfile) {
  char buffer[1024];
  utf8_t result = FileSystem::get_temporary_file_name(new_c_string("xyz"), buffer, 1024);
  ASSERT_TRUE(result.size > 0);
}
