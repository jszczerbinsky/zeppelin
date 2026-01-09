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

#include "main.h"

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

static int8_t screlu(int32_t input) {
  if (input < 0) {
    return 0;
  }
  if (input > 127) {
    return 127;
  }
  return (int8_t)input;
}

void nnue_calc_deep_acc(NNUE *nnue) {
  for (int i2 = 0; i2 < NNUE_ACC2_SIZE; i2++) {
    nnue->acc2[i2] = l2bias[i2];
    for (int i1 = 0; i1 < NNUE_ACC1_SIZE; i1++) {
      if (i2 == 0) {
      }
      int32_t shifted = nnue->acc1[i1] >> 6;
      int8_t activated = screlu(shifted);

      nnue->acc2[i2] +=
          (int32_t)activated * (int32_t)l2weight[i2 * NNUE_ACC1_SIZE + i1];
    }
  }

  for (int i3 = 0; i3 < NNUE_ACC3_SIZE; i3++) {
    nnue->acc3[i3] = l3bias[i3];
    for (int i2 = 0; i2 < NNUE_ACC2_SIZE; i2++) {
      int32_t shifted = nnue->acc2[i2] >> 6;
      int8_t activated = screlu(shifted);

      nnue->acc3[i3] +=
          (int32_t)activated * (int32_t)l3weight[i3 * NNUE_ACC2_SIZE + i2];
    }
  }

  nnue->out = l4bias[0];
  for (int i3 = 0; i3 < NNUE_ACC3_SIZE; i3++) {
    int32_t shifted = nnue->acc3[i3] >> 6;
    int8_t activated = screlu(shifted);

    nnue->out += (int32_t)activated * (int32_t)l4weight[i3];
  }
  if (nnue->out >= 0) {
    int32_t shifted = nnue->out >> 6;
    nnue->out = shifted;
  } else {
    int32_t shifted = (-nnue->out) >> 6;
    nnue->out = -shifted;
  }
}

void nnue_acc1_add(NNUE *nnue, int i0) {
  for (int i1 = 0; i1 < NNUE_ACC1_SIZE; i1++) {
    nnue->acc1[i1] += (int32_t)l1weight[i1 * NNUE_ACC0_SIZE + i0];
  }
}

void nnue_acc1_sub(NNUE *nnue, int i0) {
  for (int i1 = 0; i1 < NNUE_ACC1_SIZE; i1++) {
    nnue->acc1[i1] -= (int32_t)l1weight[i1 * NNUE_ACC0_SIZE + i0];
  }
}

void nnue_init(NNUE *nnue) {
  for (int i = 0; i < NNUE_ACC1_SIZE; i++) {
    nnue->acc1[i] = l1bias[i];
  }

  memset(nnue->acc2, 0, NNUE_ACC2_SIZE * sizeof(int32_t));
  memset(nnue->acc3, 0, NNUE_ACC3_SIZE * sizeof(int32_t));
  nnue->out = 0;

  for (int sqr = 0; sqr < 64; sqr++) {
    for (int p = 0; p < PIECE_MAX; p++) {
      int isw = (g_game.pieces[WHITE][p] & sqr2bbrd(sqr)) == sqr2bbrd(sqr);
      int isb = (g_game.pieces[BLACK][p] & sqr2bbrd(sqr)) == sqr2bbrd(sqr);

      int idx_w = NNUE_IN_IDX(WHITE, sqr, p);
      int idx_b = NNUE_IN_IDX(BLACK, sqr, p);

      if (isw) {
        nnue->acc0[idx_w] = 1;
        nnue->acc0[idx_b] = 0;
        nnue_acc1_add(nnue, idx_w);
      } else if (isb) {
        nnue->acc0[idx_w] = 0;
        nnue->acc0[idx_b] = 1;
        nnue_acc1_add(nnue, idx_b);
      } else {
        nnue->acc0[idx_w] = 0;
        nnue->acc0[idx_b] = 0;
      }
    }
  }

  nnue_calc_deep_acc(nnue);
}
