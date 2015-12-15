//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_UTILS_BLOB_HH
#define _TCLIB_UTILS_BLOB_HH

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "utils/blob.h"
END_C_INCLUDES

namespace tclib {

class Blob : public blob_t {
public:
  // Creates a new empty blob.
  Blob() { *static_cast<blob_t*>(this) = blob_empty(); }

  // Creates a new blob with the given contents.
  Blob(void *start, size_t size) { *static_cast<blob_t*>(this) = blob_new(start, size); }

  void *start() { return blob_t::start; }

  void *end() { return static_cast<byte_t*>(start()) + size(); }

  size_t size() { return blob_t::size; }

};

} // namespace tclib

#endif // _TCLIB_UTILS_BLOB_HH
