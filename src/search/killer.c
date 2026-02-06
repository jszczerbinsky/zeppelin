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

#include <string.h>

#include "../core/movelist.h"
#include "killer.h"

static Move killers[MAX_PLY_PER_GAME][KILLER_MAX];

void addkiller(int depth, Move move) {
  for (int i = 0; i < KILLER_MAX; i++) {
    if (killers[depth][i] == move)
      return;
    if (killers[depth][i] == NULLMOVE) {
      killers[depth][i] = move;
      return;
    }
  }
}

int iskiller(int depth, Move move) {
  for (int i = 0; i < KILLER_MAX; i++)
    if (killers[depth][i] == move)
      return 1;
  return 0;
}

void clearkiller(void) {
  memset(killers, 0, sizeof(Move) * KILLER_MAX * MAX_PLY_PER_GAME);
}
