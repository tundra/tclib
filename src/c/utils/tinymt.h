//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

/// Tiny Mersenne twister; based on
/// http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/TINYMT/index.html. Rewritten
/// from scratch but heavily influenced by TinyMT-src from that same source.

#include "stdc.h"

// Static Mersenne twister parameters. Unlike the seed these have to be
// generated carefully for the twister to have the right properties. Typically
// you'll want them to be baked into the binary.
typedef struct {
  uint32_t transition_matrix_1;
  uint32_t transition_matrix_2;
  uint64_t tempering_matrix;
} tinymt64_params_t;

// Dynamic state that changes for each new value returned from the twister.
// It's wrapped in a struct (ideally it would be a typedef) to make by-value
// assignment work.
typedef struct {
  uint64_t data[2];
} tinymt64_state_t;

// The internal state of a mersenne twister.
typedef struct {
  // Current "hidden" state.
  tinymt64_state_t state;
  // Constant parameters.
  tinymt64_params_t params;
} tinymt64_t;

// Initialize the tiny twister state based on the given seed.
tinymt64_t new_tinymt64(tinymt64_params_t params, uint64_t seed);

// Returns the default parameters. These are known to produce a decent output.
tinymt64_params_t tinymt64_params_default();

// Returns the next uint64_t generated from the given Mersenne twister, storing
// the new internal state through the state output parameter. It's fine for the
// output state to point to the other parameter's state if that's convenient
// but it's not a requirement.
uint64_t tinymt64_next_uint64(const tinymt64_t *tinymt, tinymt64_state_t *state_out);
