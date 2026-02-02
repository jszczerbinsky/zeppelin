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

BitBrd hash_piecesqr[2][64][PIECE_MAX];
BitBrd hash_whitemove;
BitBrd hash_castle_wk;
BitBrd hash_castle_wq;
BitBrd hash_castle_bk;
BitBrd hash_castle_bq;
BitBrd hash_epfile[8];

void inithash(void) {
  for (int player = 0; player < 2; player++) {
    hash_whitemove = rand64();
    for (int piece = 0; piece < PIECE_MAX; piece++)
      for (int sqr = 0; sqr < 64; sqr++)
        hash_piecesqr[player][sqr][piece] = rand64();
  }
  hash_castle_wk = rand64();
  hash_castle_wq = rand64();
  hash_castle_bk = rand64();
  hash_castle_bq = rand64();
  for (int i = 0; i < 8; i++)
    hash_epfile[i] = rand64();
}

BitBrd gethash(void) {
  BitBrd hash = 0;

  if (g_game.who2move == WHITE) {
    hash ^= hash_whitemove;
  }

  for (int sqr = 0; sqr < 64; sqr++) {
    BitBrd bbrd = sqr2bbrd(sqr);

    for (int player = 0; player < 2; player++)
      for (int piece = 0; piece < PIECE_MAX; piece++)
        if (g_game.pieces[player][piece] & bbrd)
          hash ^= hash_piecesqr[player][sqr][piece];

    if (g_gamestate->epbbrd & bbrd)
      hash ^= hash_epfile[sqr % 8];
  }

  if (CANCASTLE_WK(g_gamestate))
    hash ^= hash_castle_wk;
  if (CANCASTLE_WQ(g_gamestate))
    hash ^= hash_castle_wq;
  if (CANCASTLE_BK(g_gamestate))
    hash ^= hash_castle_bk;
  if (CANCASTLE_BQ(g_gamestate))
    hash ^= hash_castle_bq;

  return hash;
}
