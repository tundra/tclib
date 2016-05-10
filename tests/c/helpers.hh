//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/stdc.h"

BEGIN_C_INCLUDES
#include "utils/string-inl.h"
END_C_INCLUDES

// Various utilities that are convenient when testing tclib.

// Returns the path to the durian executable.
utf8_t get_durian_main();

// Returns the path to the injectee dll.
utf8_t get_injectee_dll();
