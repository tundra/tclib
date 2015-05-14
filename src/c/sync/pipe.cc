//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "sync/pipe.hh"

BEGIN_C_INCLUDES
#include "utils/log.h"
END_C_INCLUDES

using namespace tclib;

NativePipe::NativePipe()
  : in_(NULL)
  , out_(NULL) {
}

NativePipe::~NativePipe() {
  if (in_ != NULL) {
    delete in_;
    in_ = NULL;
  }
  if (out_ != NULL) {
    delete out_;
    out_ = NULL;
  }
}

#ifdef IS_GCC
#  include "pipe-posix.cc"
#endif

#ifdef IS_MSVC
#  include "pipe-msvc.cc"
#endif
