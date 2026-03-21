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

#ifndef PRECOMP_H
#define PRECOMP_H

#include "bitboard.h"

typedef struct {
  const BitBrd *knightmask;
  const BitBrd *kingmask;
  const BitBrd *bishoppremask;
  const BitBrd *bishoppostmask;
  const BitBrd *rookpremask;
  const BitBrd *rookpostmask;
  const BitBrd *queenpremask;
  const BitBrd *queenpostmask;
  const BitBrd *pawnattackmask[2];

#ifdef BIT_NONE
  const uint8_t *rookmagicshift;
  const uint8_t *bishopmagicshift;
  const BitBrd *rookmagic;
  const BitBrd *bishopmagic;
  const BitBrd *rookmagicmoves[64];
  const BitBrd *bishopmagicmoves[64];
#endif
#ifdef BIT_BMI2
  const uint16_t *rookpextsize;
  const uint16_t *bishoppextsize;
  const BitBrd *rookpextmoves[64];
  const BitBrd *bishoppextmoves[64];
#endif
} PrecompTable;

extern PrecompTable g_precomp;

int initprecomp(void);

#endif
