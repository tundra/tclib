//- Copyright 2015 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#ifndef _TCLIB_CALLBACK_H
#define _TCLIB_CALLBACK_H

// C bindings for the C++ callbacks.
//
// The C++ callbacks rely heavily on templates and so there's more work in
// creating the C version. The naming conventions are as follows: a callback's
// type is named after the return type, then "callback", then each of the
// arguments, then "_t". So voidp_callback_voidp_int_t is a callback that takes
// a voidp (that is, void*) and an int as arguments and returns voidp*. The
// constructors use the same convention but also include a number that indicates
// how many bound arguments there are. So new_voidp_callback_voidp_1 takes a
// void*(void*) and 1 bound argument, which will be a void*. Note that his means
// that the constructor is named for the function they bind, not the type they
// return (because those aren't unique).

#include "c/stdc.h"

// void*(void)
typedef struct voidp_callback_t voidp_callback_t;
voidp_callback_t *voidp_callback_0(void *(invoker)(void));
void *voidp_callback_call(voidp_callback_t *callback);

// void*(void*)
typedef struct voidp_callback_voidp_t voidp_callback_voidp_t;
voidp_callback_voidp_t *voidp_callback_voidp_0(void *(invoker)(void*));
voidp_callback_t *voidp_callback_voidp_1(void *(invoker)(void*), void *b0);
void *voidp_callback_voidp_call(voidp_callback_voidp_t *callback, void *a0);

// Deletes the given callback. This function works on all callbacks, regardless
// of their concrete type.
void callback_dispose(void *callback);

#endif // _TCLIB_CALLBACK_H
