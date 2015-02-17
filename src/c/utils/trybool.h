//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_C_TRYBOOL
#define _TCLIB_C_TRYBOOL

// Evaluate the given expression, if it returns false then return false from the
// current context.
#define B_TRY(EXPR) do {                                                       \
  if (!(EXPR))                                                                 \
    return false;                                                              \
} while (false)

#endif // _TCLIB_C_TRYBOOL
