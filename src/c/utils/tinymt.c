//- Copyright 2014 the Neutrino authors (see AUTHORS).
//- Licensed under the Apache License, Version 2.0 (see LICENSE).

#include "tinymt.h"

#define kMask63Bits 0x7FFFFFFFFFFFFFFFULL

// Tempers the output.
static uint64_t tinymt64_temper(const tinymt64_params_t *params,
    tinymt64_state_t *state) {
  uint64_t x = state->data[0] + state->data[1];
  x ^= state->data[0] >> 8;
  x ^= ((uint64_t) (-((int64_t) (x & 1)))) & params->tempering_matrix;
  return x;
}

// Transitions the twister to the next state.
static void tinymt64_transition(const tinymt64_t *tinymt, tinymt64_state_t *state_out) {
  tinymt64_state_t copy = tinymt->state;
  copy.data[0] &= kMask63Bits;
  uint64_t x = copy.data[0] ^ copy.data[1];
  x ^= x << 12;
  x ^= x >> 32;
  x ^= x << 32;
  x ^= x << 11;
  copy.data[0] = copy.data[1];
  copy.data[1] = x;
  copy.data[0] ^= ((uint64_t) -((int64_t)(x & 1))) & tinymt->params.transition_matrix_1;
  copy.data[1] ^= ((uint64_t) -((int64_t)(x & 1))) & (((uint64_t) tinymt->params.transition_matrix_2) << 32);
  *state_out = copy;
}

static void period_certification(tinymt64_t *tinymt) {
  if ((tinymt->state.data[0] & kMask63Bits) == 0 && (tinymt->state.data[1] == 0)) {
    tinymt->state.data[0] = 'T';
    tinymt->state.data[1] = 'M';
  }
}

tinymt64_t tinymt64_construct(tinymt64_params_t params, uint64_t seed) {
  tinymt64_t tinymt;
  tinymt.params = params;
  tinymt.state.data[0] = seed ^ (((uint64_t) params.transition_matrix_1) << 32);
  tinymt.state.data[1] = params.transition_matrix_2 ^ params.tempering_matrix;
  for (size_t i = 1; i < 8; i++) {
    uint64_t factor = tinymt.state.data[(i - 1) & 1] ^ (tinymt.state.data[(i - 1) & 1] >> 62);
    tinymt.state.data[i & 1] ^= i + (6364136223846793005ULL * factor);
  }
  period_certification(&tinymt);
  return tinymt;
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

uint64_t tinymt64_next_uint64(const tinymt64_t *tinymt, tinymt64_state_t *state_out) {
  tinymt64_transition(tinymt, state_out);
  return tinymt64_temper(&tinymt->params, state_out);
}
