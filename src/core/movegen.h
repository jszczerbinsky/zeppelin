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

#define GEN_ALL 0
#define GEN_QUIET 1
#define GEN_CAPT 2

// checks_cnt is optional, can use 0, but correct value will reduce time
void gen_moves(int player, MoveList *movelist, BitBrd *attackbbrd, int movetype,
               int checks_cnt);

int get_sqr_attackers_cnt(int attacker, int sqr);

static inline int lastmovelegal(void) {
  if (GET_CAPT_PIECE(g_game.movelist.move[g_game.movelist.cnt - 1]) == KING) {
    return 0;
  }

  return get_sqr_attackers_cnt(
             g_game.who2move,
             bbrd2sqr(g_game.pieces[!g_game.who2move][KING])) == 0;
}

static inline int get_under_check_cnt(void) {
  return get_sqr_attackers_cnt(!g_game.who2move,
                               bbrd2sqr(g_game.pieces[g_game.who2move][KING]));
}

static inline int giving_check_cnt(void) {
  return get_sqr_attackers_cnt(g_game.who2move,
                               bbrd2sqr(g_game.pieces[!g_game.who2move][KING]));
}

#endif
