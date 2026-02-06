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

#ifndef TT_H
#define TT_H

#include "../core/bitboard.h"
#include "../core/move.h"

#define TT_EXACT 0
#define TT_LOWERBOUND 1
#define TT_UPPERBOUND 2

typedef struct {
  int used;
  BitBrd hash;
  int type;
  int value;
  int depth;
  Move bestmove;
  int searchid;
} TT;

void ttinit(void);
void ttfree(void);

size_t getttsize(void);
size_t getttused(void);

const TT *ttread(BitBrd hash, int depth, int searchid);
void ttwrite(BitBrd hash, int type, int depth, int value, Move bestmove,
             int searchid);

#endif
