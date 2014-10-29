//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "tinymt.h"

#define kMask63Bits 0x7FFFFFFFFFFFFFFFULL

// Tempers the output.
static uint64_t tinymt64_temper(tinymt64_t *state) {
  uint64_t x = state->status[0] + state->status[1];
  x ^= state->status[0] >> 8;
  x ^= -((int64_t) (x & 1)) & state->params.tempering_matrix;
  return x;
}

// Transitions the twister to the next state.
static void tinymt64_transition(tinymt64_t *state) {
  state->status[0] &= kMask63Bits;
  uint64_t x = state->status[0] ^ state->status[1];
  x ^= x << 12;
  x ^= x >> 32;
  x ^= x << 32;
  x ^= x << 11;
  state->status[0] = state->status[1];
  state->status[1] = x;
  state->status[0] ^= -((int64_t)(x & 1)) & state->params.transition_matrix_1;
  state->status[1] ^= -((int64_t)(x & 1)) & (((uint64_t) state->params.transition_matrix_2) << 32);
}

static void period_certification(tinymt64_t * state) {
  if ((state->status[0] & kMask63Bits) == 0 && (state->status[1] == 0)) {
    state->status[0] = 'T';
    state->status[1] = 'M';
  }
}

void tinymt64_init(tinymt64_t *state, tinymt64_params_t params, uint64_t seed) {
  state->params = params;
  state->status[0] = seed ^ (((uint64_t) state->params.transition_matrix_1) << 32);
  state->status[1] = state->params.transition_matrix_2 ^ state->params.tempering_matrix;
  for (size_t i = 1; i < 8; i++) {
    uint64_t factor = state->status[(i - 1) & 1] ^ (state->status[(i - 1) & 1] >> 62);
    state->status[i & 1] ^= i + (6364136223846793005ULL * factor);
  }
  period_certification(state);
}

tinymt64_params_t tinymt64_params_default() {
  // This set of parameters are taken from the pre-generated tinymt64dc output
  // at http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/TINYMT/DATA/index.html.
  //
  // 99db8a45f093f06368a38c537afa159b,64,0,fd581fab,8c982326,bd7fc6ffefffffbc,65,0
  tinymt64_params_t result;
  result.transition_matrix_1 = 0xfd581fab;
  result.transition_matrix_2 = 0x8c982326;
  result.tempering_matrix = 0xbd7fc6ffefffffbcULL;
  return result;
}

uint64_t tinymt64_next_uint64(tinymt64_t *state) {
  tinymt64_transition(state);
  return tinymt64_temper(state);
}
