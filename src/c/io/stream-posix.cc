//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <unistd.h>

// It's a coincidence that the convention on both platforms happens to be -1.
naked_file_handle_t AbstractStream::kNullNakedFileHandle = -1;

bool AbstractStream::is_a_tty() {
  naked_file_handle_t handle = to_raw_handle();
  if (handle == kNullNakedFileHandle)
    return false;
  return isatty(handle);
}
