//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_CALLBACK_H
#define _TCLIB_CALLBACK_H

// C bindings for the C++ callbacks.
//
// The C++ callbacks rely heavily on templates and so there's more work in
// creating the C version. To avoid a combinatorical explosion of bound/unbound
// arguments plus argument types I've tried to stick with the generic opaque
// type and rely on casting instead.
//
// The naming convention is as follows. The types are named for how many
// arguments they expect: nullary take no arguments, unary take one, etc. The
// constructors are named after how many arguments they bind so, for instance,
// binary_callback_new_3 would take a 5-argument function, bind 3 of the
// arguments (the "_3"), and yield a result that expects 2 additional arguments
// (the "binary").

#include "c/stdc.h"

#include "utils/opaque.h"

// opaque_t(void)
typedef struct nullary_callback_t nullary_callback_t;
nullary_callback_t *nullary_callback_new_0(opaque_t (invoker)(void));
nullary_callback_t *nullary_callback_new_1(opaque_t (invoker)(opaque_t), opaque_t b0);
nullary_callback_t *nullary_callback_new_2(opaque_t (invoker)(opaque_t, opaque_t),
    opaque_t b0, opaque_t b1);
opaque_t nullary_callback_call(nullary_callback_t *callback);

// opaque_t(opaque_t)
typedef struct unary_callback_t unary_callback_t;
unary_callback_t *unary_callback_new_0(opaque_t (invoker)(opaque_t));
unary_callback_t *unary_callback_new_1(opaque_t (invoker)(opaque_t, opaque_t),
    opaque_t b0);
unary_callback_t *unary_callback_new_2(opaque_t (invoker)(opaque_t, opaque_t, opaque_t),
    opaque_t b0, opaque_t b1);
opaque_t unary_callback_call(unary_callback_t *callback, opaque_t a0);

// Deletes the given callback. This function works on all callbacks, regardless
// of their concrete type.
void callback_destroy(void *callback);

#endif // _TCLIB_CALLBACK_H
