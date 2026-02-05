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

#ifndef MOVELIST_H
#define MOVELIST_H

#include "move.h"
#include "piece.h"

#define MAX_PLY_PER_GAME 512

typedef struct {
  Move move[MAX_PLY_PER_GAME];
  int cnt;
} MoveList;

static inline void popmove(MoveList *moves) { moves->cnt--; }

static inline void pushmove(MoveList *moves, Move m) {
  moves->move[moves->cnt] = m;
  moves->cnt++;
}

static inline void pushprommove(MoveList *moves, Move m) {
  for (int prompiece = KNIGHT; prompiece <= QUEEN; prompiece++) {
    moves->move[moves->cnt] = m | PROM_PIECE(prompiece);
    moves->cnt++;
  }
}

static inline int containsmove(const MoveList *moves, Move m) {
  for (int i = 0; i < moves->cnt; i++) {
    if (m == moves->move[i]) {
      return 1;
    }
  }
  return 0;
}

#endif
