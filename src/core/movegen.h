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

#ifndef MOVEGEN_H
#define MOVEGEN_H

#include "bitboard.h"
#include "game.h"
#include "movelist.h"

// checks_cnt is optional, can use 0, but correct value will reduce time
void gen_moves(int player, MoveList *movelist);

Move make_lva(int attacker, int sqr);

int is_sqr_attacked(int attacker, int sqr);

static inline int lastmovelegal(void) {
  if (GET_CAPT_PIECE(g_game.movelist.move[g_game.movelist.cnt - 1]) == KING) {
    return 0;
  }

  return is_sqr_attacked(g_game.who2move,
                         bbrd2sqr(g_game.pieces[!g_game.who2move][KING])) == 0;
}

static inline int is_under_check(void) {
  return is_sqr_attacked(!g_game.who2move,
                         bbrd2sqr(g_game.pieces[g_game.who2move][KING]));
}

static inline int is_giving_check(void) {
  return is_sqr_attacked(g_game.who2move,
                         bbrd2sqr(g_game.pieces[!g_game.who2move][KING]));
}

int is_promotion_available(int player);

#endif
