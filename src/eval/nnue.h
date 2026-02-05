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

#ifndef NNUE_H
#define NNUE_H

#include <stdalign.h>
#include <stdint.h>

#include "../core/piece.h"

#define NNUE_IN_IDX(c, sqr, p) (PIECE_MAX * 64 * (c) + PIECE_MAX * (sqr) + p)

#define NNUE_ACC0_SIZE (64 * 2 * PIECE_MAX)

#include "../../res/nnue_shape.h"

#define NNUE_L1_SIZE (NNUE_ACC0_SIZE * NNUE_ACC1_SIZE)
#define NNUE_L2_SIZE (NNUE_ACC1_SIZE * NNUE_ACC2_SIZE)
#define NNUE_L3_SIZE (NNUE_ACC2_SIZE * NNUE_ACC3_SIZE)
#define NNUE_L4_SIZE (NNUE_ACC3_SIZE * 1)

#define NNUE_CACHE_SIZE 16

typedef struct {
  // Always from white perspective
  int8_t acc0[NNUE_ACC0_SIZE];
  alignas(32) int32_t acc1[NNUE_ACC1_SIZE];
  // int32_t acc2[NNUE_ACC2_SIZE];
  // int32_t acc3[NNUE_ACC3_SIZE];
  // int32_t out;

  int cache_add[NNUE_CACHE_SIZE];
  int cache_sub[NNUE_CACHE_SIZE];

  int cache_addcnt;
  int cache_subcnt;
} NNUE;

void nnue_init(NNUE *nnue);
void nnue_release_cache(NNUE *nnue);
void nnue_acc1_add(NNUE *nnue, int i0);
void nnue_acc1_sub(NNUE *nnue, int i0);
void nnue_load_weights(void);
void nnue_calc_deep_acc(NNUE *nnue);

#endif
