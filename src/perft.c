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

void perft(int depth, int *nodes, int *leafnodes) {
  if (depth == 0) {
    (*nodes)++;
    (*leafnodes)++;
    return;
  }

  MoveList movelist;
  BitBrd attacksbbrd;
  gen_moves(g_game.who2move, &movelist, &attacksbbrd, GEN_ALL, 0);

  if (movelist.cnt == 0) {
    (*nodes)++;
    (*leafnodes)++;
    return;
  }

  for (int i = 0; i < movelist.cnt; i++) {
    makemove(movelist.move[i]);

    if (lastmovelegal())
      perft(depth - 1, nodes, leafnodes);
    unmakemove();
  }
}
