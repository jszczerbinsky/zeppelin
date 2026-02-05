/*
 * Zeppelin chess engine.
 *
 * Copyright (C) 2024-2026 Jakub Szczerbiński <jszczerbinsky2@gmail.com>
 *
 * Zeppelin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifdef VECT_AVX2
#include <immintrin.h>
#endif

#ifdef VECT_NEON
#include <arm_neon.h>
#endif

#include <string.h>

#include "../core/game.h"
#include "nnue.h"

extern const unsigned char _binary_nnue_weights_bin_start[];
extern const unsigned char _binary_nnue_bias_bin_start[]
    __attribute__((aligned(4)));

#define NNUE_L1W_START 0
#define NNUE_L2W_START (NNUE_L1W_START + NNUE_L1_SIZE)
#define NNUE_L3W_START (NNUE_L2W_START + NNUE_L2_SIZE)
#define NNUE_L4W_START (NNUE_L3W_START + NNUE_L3_SIZE)

#define NNUE_L1B_START 0
#define NNUE_L2B_START (NNUE_L1B_START + NNUE_ACC1_SIZE * 4)
#define NNUE_L3B_START (NNUE_L2B_START + NNUE_ACC2_SIZE * 4)
#define NNUE_L4B_START (NNUE_L3B_START + NNUE_ACC3_SIZE * 4)

typedef int8_t L1WeightRow[NNUE_ACC0_SIZE];
typedef int8_t L2WeightRow[NNUE_ACC1_SIZE];
typedef int8_t L3WeightRow[NNUE_ACC2_SIZE];
typedef int8_t L4WeightRow[NNUE_ACC3_SIZE];

const int8_t *l1weight =
    (const int8_t *)(_binary_nnue_weights_bin_start + NNUE_L1W_START);
const int8_t *l2weight =
    (const int8_t *)(_binary_nnue_weights_bin_start + NNUE_L2W_START);
const int8_t *l3weight =
    (const int8_t *)(_binary_nnue_weights_bin_start + NNUE_L3W_START);
const int8_t *l4weight =
    (const int8_t *)(_binary_nnue_weights_bin_start + NNUE_L4W_START);

const int32_t *l1bias =
    (const int32_t *)(_binary_nnue_bias_bin_start + NNUE_L1B_START);
const int32_t *l2bias =
    (const int32_t *)(_binary_nnue_bias_bin_start + NNUE_L2B_START);
const int32_t *l3bias =
    (const int32_t *)(_binary_nnue_bias_bin_start + NNUE_L3B_START);
const int32_t *l4bias =
    (const int32_t *)(_binary_nnue_bias_bin_start + NNUE_L4B_START);

alignas(32) static int32_t l1weight32[NNUE_ACC0_SIZE][NNUE_ACC1_SIZE];

static void add_weights(int32_t *acc_arr, int acc_size, int8_t *prev_acc_arr,
                        int prev_acc_size, const int8_t *weights) {
#ifdef VECT_NONE
  for (int l2 = 0; l2 < acc_size; l2++) {
    for (int l1 = 0; l1 < prev_acc_size; l1++) {
      acc_arr[l2] +=
          (int32_t)prev_acc_arr[l1] * (int32_t)weights[l2 * prev_acc_size + l1];
    }
  }
#endif

#ifdef VECT_AVX2
  int chunk = prev_acc_size / 32;

  for (int i2 = 0; i2 < acc_size; i2++) {
    for (int i = 0; i < chunk; i++) {
      __m256i prev_acc = _mm256_load_si256(((const __m256i *)prev_acc_arr) + i);

      __m256i w = _mm256_loadu_si256(
          (const __m256i *)(weights + i2 * prev_acc_size + i * 32));

      __m256i w16_lo = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(w));
      __m256i a16_lo = _mm256_cvtepi8_epi16(_mm256_castsi256_si128(prev_acc));
      __m256i w16_hi = _mm256_cvtepi8_epi16(_mm256_extracti128_si256(w, 1));
      __m256i a16_hi =
          _mm256_cvtepi8_epi16(_mm256_extracti128_si256(prev_acc, 1));

      __m256i res_lo = _mm256_madd_epi16(w16_lo, a16_lo);
      __m256i res_hi = _mm256_madd_epi16(w16_hi, a16_hi);

      __m256i res = _mm256_add_epi32(res_lo, res_hi);

      __m128i res128 = _mm_add_epi32(_mm256_extracti128_si256(res, 1),
                                     _mm256_castsi256_si128(res));
      res128 = _mm_add_epi32(
          res128, _mm_shuffle_epi32(res128, _MM_SHUFFLE(0, 1, 2, 3)));
      res128 = _mm_add_epi32(
          res128, _mm_shuffle_epi32(res128, _MM_SHUFFLE(0, 0, 0, 1)));

      acc_arr[i2] += _mm_cvtsi128_si32(res128);
    }
  }
#endif

#ifdef VECT_NEON
  int chunk = prev_acc_size / 16;
  int8x16_t prev_acc[chunk];
  for (int i = 0; i < chunk; i++) {
    prev_acc[i] = vld1q_s8(prev_acc_arr + i * 16);
  }

  for (int i2 = 0; i2 < acc_size; i2++) {
    for (int i = 0; i < chunk; i++) {
      int8x16_t w = vld1q_s8(weights + i2 * prev_acc_size + i * 16);

      int8x8_t w_lo = vget_low_s8(w);
      int8x8_t a_lo = vget_low_s8(prev_acc[i]);
      int8x8_t w_hi = vget_high_s8(w);
      int8x8_t a_hi = vget_high_s8(prev_acc[i]);

      int16x8_t res_lo = vmull_s8(w_lo, a_lo);
      int16x8_t res_hi = vmull_s8(w_hi, a_hi);

      int32x4_t res = vdupq_n_s32(0);
      res = vaddq_s32(res, vpaddlq_s16(res_lo));
      res = vaddq_s32(res, vpaddlq_s16(res_hi));

      acc_arr[i2] += vaddvq_s32(res);
    }
  }
#endif
}

static void activate(int8_t *dest, int32_t *acc, int acc_size) {
#ifdef VECT_NONE
  for (int i = 0; i < acc_size; i++) {
    int32_t res = acc[i] >> 6;

    if (res < 0)
      dest[i] = 0;
    else if (res > 127)
      dest[i] = 127;
    else
      dest[i] = (int8_t)res;
  }
#endif

#ifdef VECT_AVX2
  __m256i zeroes = _mm256_set1_epi32(0);
  __m256i max = _mm256_set1_epi32(127);

  int chunk2 = acc_size / 8;
  for (int i = 0; i < chunk2; i++) {
    __m256i a = _mm256_load_si256((const __m256i *)(acc + i * 8));
    a = _mm256_srai_epi32(a, 6);
    a = _mm256_max_epi32(a, zeroes);
    a = _mm256_min_epi32(a, max);

    __m128i a_lo = _mm256_castsi256_si128(a);
    __m128i a_hi = _mm256_extracti128_si256(a, 1);
    __m128i a16 = _mm_packs_epi32(a_lo, a_hi);
    __m128i a8 = _mm_packs_epi16(a16, a16);

    _mm_storel_epi64((__m128i *)(dest + i * 8), a8);
  }
#endif

#ifdef VECT_NEON
  int32x4_t zeroes = vdupq_n_s32(0);
  int32x4_t max = vdupq_n_s32(127);

  int32_t activated[acc_size];

  int chunk2 = acc_size / 4;
  for (int i = 0; i < chunk2; i++) {
    int32x4_t a = vld1q_s32(acc + i * 4);
    a = vshrq_n_s32(a, 6);
    a = vmaxq_s32(a, zeroes);
    a = vminq_s32(a, max);
    vst1q_s32(activated + i * 4, a);
  }
  for (int i = 0; i < acc_size; i++) {
    dest[i] = (int8_t)activated[i];
  }
#endif
}

void nnue_calc_deep_acc(NNUE *nnue) {
  nnue_release_cache(nnue);

  alignas(32) int8_t acc1_act[NNUE_ACC1_SIZE];
  activate(acc1_act, nnue->acc1, NNUE_ACC1_SIZE);

  alignas(32) int32_t acc2[NNUE_ACC2_SIZE];
  memcpy(acc2, l2bias, NNUE_ACC2_SIZE * sizeof(int32_t));
  add_weights(acc2, NNUE_ACC2_SIZE, acc1_act, NNUE_ACC1_SIZE, l2weight);
  alignas(32) int8_t acc2_act[NNUE_ACC2_SIZE];
  activate(acc2_act, acc2, NNUE_ACC2_SIZE);

  alignas(32) int32_t acc3[NNUE_ACC3_SIZE];
  memcpy(acc3, l3bias, NNUE_ACC3_SIZE * sizeof(int32_t));
  add_weights(acc3, NNUE_ACC3_SIZE, acc2_act, NNUE_ACC2_SIZE, l3weight);
  alignas(32) int8_t acc3_act[NNUE_ACC3_SIZE];
  activate(acc3_act, acc3, NNUE_ACC3_SIZE);

  int32_t out = l4bias[0];
  add_weights(&out, 1, acc3_act, NNUE_ACC3_SIZE, l4weight);

  int32_t shifted = out >> 6;
  out = shifted;
  g_gamestate->nnue_eval = out;
  g_gamestate->nnue_ready = 1;
}

void nnue_acc1_add_now(NNUE *nnue, int i0) {
#ifndef VECT_AVX2
  for (int i1 = 0; i1 < NNUE_ACC1_SIZE; i1++) {
    nnue->acc1[i1] += (int32_t)l1weight[i1 * NNUE_ACC0_SIZE + i0];
  }
#endif
#ifdef VECT_AVX2
  int chunk = NNUE_ACC1_SIZE / 8;
  for (int i = 0; i < chunk; i++) {
    __m256i w = _mm256_loadu_si256((__m256i *)(l1weight32[i0] + i * 8));
    __m256i a = _mm256_loadu_si256((__m256i *)(nnue->acc1 + i * 8));

    a = _mm256_add_epi32(a, w);
    _mm256_storeu_si256((__m256i *)(nnue->acc1 + i * 8), a);
  }
#endif
}

static void nnue_acc1_sub_now(NNUE *nnue, int i0) {
#ifndef VECT_AVX2
  for (int i1 = 0; i1 < NNUE_ACC1_SIZE; i1++) {
    nnue->acc1[i1] -= (int32_t)l1weight[i1 * NNUE_ACC0_SIZE + i0];
  }
#endif
#ifdef VECT_AVX2
  int chunk = NNUE_ACC1_SIZE / 8;
  for (int i = 0; i < chunk; i++) {
    __m256i w = _mm256_loadu_si256((__m256i *)(l1weight32[i0] + i * 8));
    __m256i a = _mm256_loadu_si256((__m256i *)(nnue->acc1 + i * 8));

    a = _mm256_sub_epi32(a, w);
    _mm256_storeu_si256((__m256i *)(nnue->acc1 + i * 8), a);
  }
#endif
}

void nnue_release_cache(NNUE *nnue) {
  for (int i = 0; i < nnue->cache_addcnt; i++) {
    nnue_acc1_add_now(nnue, nnue->cache_add[i]);
  }
  for (int i = 0; i < nnue->cache_subcnt; i++) {
    nnue_acc1_sub_now(nnue, nnue->cache_sub[i]);
  }

  nnue->cache_addcnt = 0;
  nnue->cache_subcnt = 0;
}

void nnue_acc1_add(NNUE *nnue, int i0) {
  for (int i = 0; i < nnue->cache_subcnt; i++) {
    if (nnue->cache_sub[i] == i0) {
      nnue->cache_subcnt--;
      nnue->cache_sub[i] = nnue->cache_sub[nnue->cache_subcnt];
      return;
    }
  }

  if (nnue->cache_addcnt + 1 >= NNUE_CACHE_SIZE)
    nnue_release_cache(nnue);

  nnue->cache_add[nnue->cache_addcnt] = i0;
  nnue->cache_addcnt++;
}

void nnue_acc1_sub(NNUE *nnue, int i0) {
  for (int i = 0; i < nnue->cache_addcnt; i++) {
    if (nnue->cache_add[i] == i0) {
      nnue->cache_addcnt--;
      nnue->cache_add[i] = nnue->cache_add[nnue->cache_addcnt];
      return;
    }
  }

  if (nnue->cache_subcnt + 1 >= NNUE_CACHE_SIZE)
    nnue_release_cache(nnue);

  nnue->cache_sub[nnue->cache_subcnt] = i0;
  nnue->cache_subcnt++;
}

void nnue_load_weights(void) {
  for (int i1 = 0; i1 < NNUE_ACC1_SIZE; i1++) {
    for (int i0 = 0; i0 < NNUE_ACC0_SIZE; i0++) {
      l1weight32[i0][i1] = (int32_t)l1weight[i1 * NNUE_ACC0_SIZE + i0];
    }
  }
}

void nnue_init(NNUE *nnue) {
  for (int i = 0; i < NNUE_ACC1_SIZE; i++) {
    nnue->acc1[i] = l1bias[i];
  }

  // nnue->out = 0;

  for (int sqr = 0; sqr < 64; sqr++) {
    for (int p = 0; p < PIECE_MAX; p++) {
      int isw = (g_game.pieces[WHITE][p] & sqr2bbrd(sqr)) == sqr2bbrd(sqr);
      int isb = (g_game.pieces[BLACK][p] & sqr2bbrd(sqr)) == sqr2bbrd(sqr);

      int idx_w = NNUE_IN_IDX(WHITE, sqr, p);
      int idx_b = NNUE_IN_IDX(BLACK, sqr, p);

      if (isw) {
#ifdef DEBUG_INTERFACE
        nnue->acc0[idx_w] = 1;
        nnue->acc0[idx_b] = 0;
#endif
        nnue_acc1_add_now(nnue, idx_w);
      } else if (isb) {
#ifdef DEBUG_INTERFACE
        nnue->acc0[idx_w] = 0;
        nnue->acc0[idx_b] = 1;
#endif
        nnue_acc1_add_now(nnue, idx_b);
      }
#ifdef DEBUG_INTERFACE
      else {
        nnue->acc0[idx_w] = 0;
        nnue->acc0[idx_b] = 0;
      }
#endif
    }
  }

  nnue->cache_addcnt = 0;
  nnue->cache_subcnt = 0;
  nnue_calc_deep_acc(nnue);
}
