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
  BitBrd knightmask[64];
  BitBrd kingmask[64];
  BitBrd bishoppremask[64];
  BitBrd bishoppostmask[64];
  BitBrd rookpremask[64];
  BitBrd rookpostmask[64];
  BitBrd queenpremask[64];
  BitBrd queenpostmask[64];
  BitBrd pawnattackmask[2][64];

  int rookmagicshift[64];
  int bishopmagicshift[64];
  BitBrd rookmagic[64];
  BitBrd bishopmagic[64];

  // Serialized dynamically - n is not constant
  // BitBrd rookmagicmoves[64][n];
  // BitBrd bishopmagicmoves[64][n];

} PrecompTableSerialized;

typedef struct {
  BitBrd knightmask[64];
  BitBrd kingmask[64];
  BitBrd bishoppremask[64];
  BitBrd bishoppostmask[64];
  BitBrd rookpremask[64];
  BitBrd rookpostmask[64];
  BitBrd queenpremask[64];
  BitBrd queenpostmask[64];
  BitBrd pawnattackmask[2][64];

  int rookmagicshift[64];
  int bishopmagicshift[64];
  BitBrd rookmagic[64];
  BitBrd bishopmagic[64];

  BitBrd *rookmagicmoves[64];
  BitBrd *bishopmagicmoves[64];

} PrecompTable;

extern PrecompTable g_precomp;

void genprecomp(void);
void huntmagic(void);
void usemagic(const char *numstr);
int loadprecomp(void);
void freeprecomp(void);

#endif
