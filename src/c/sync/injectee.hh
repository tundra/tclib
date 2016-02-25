//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _INJECTEE_HH
#define _INJECTEE_HH

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "utils/alloc.h"
END_C_INCLUDES

// Combines a file id, a line and an error code into a single int that can be
// returned from an injected dll's connect function.
#define CONNECT_FAILED_RESULT(FID, LID, ERROR) ((((FID) & 0xFFF) << 20) | (((LID) & 0x3FF) << 10) | ((ERROR) & 0x3FF))

// Returns an int that indicates success when returned from an injected dll's
// connect function.
#define CONNECT_SUCCEEDED_RESULT() 0

#define CONNECTOR_IMPL(Name, DATA_IN_NAME, DATA_OUT_NAME)                                                   \
  extern "C" __declspec(dllexport) dword_t Name(blob_t DATA_IN_NAME, blob_t DATA_OUT_NAME)

#endif // _INJECTEE_HH
