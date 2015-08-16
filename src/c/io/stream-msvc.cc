//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include <io.h>

naked_file_handle_t AbstractStream::kNullNakedFileHandle = reinterpret_cast<naked_file_handle_t>(-1);

bool AbstractStream::is_a_tty() {
  naked_file_handle_t handle = to_raw_handle();
  if (handle == kNullNakedFileHandle)
    return false;
  int fd = _open_osfhandle(reinterpret_cast<intptr_t>(handle), 0);
  // TODO: does this leak file handles? The thing is, if we _close the fd then
  //   the underlying handle also gets closed which definitely isn't what we
  //   want.
  return (fd != -1) && _isatty(fd);
}
