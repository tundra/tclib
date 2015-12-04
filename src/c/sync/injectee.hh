//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _INJECTEE_HH
#define _INJECTEE_HH

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "utils/alloc.h"
END_C_INCLUDES

#define CONNECTOR_IMPL(Name, DATA_IN_NAME, DATA_OUT_NAME)                                                   \
  extern "C" __declspec(dllexport) dword_t Name(blob_t DATA_IN_NAME, blob_t *DATA_OUT_NAME)

#endif // _INJECTEE_HH
