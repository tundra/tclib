//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "c/stdc.h"
#include "utils/string-inl.h"

// Various utilities that are convenient when testing tclib.

// Returns the path to the durian executable.
utf8_t get_durian_main();

// Returns the path to the injectee dll.
utf8_t get_injectee_dll();
