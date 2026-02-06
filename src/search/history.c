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

#include "../utils/cmputils.h"
#include "history.h"

int historyoverflow = 0;
int history[2][64][64] = {0};

void addhistory(int player, Move m, int diff) {
  history[player][GET_SRC_SQR(m)][GET_DST_SQR(m)] += diff;
  history[player][GET_SRC_SQR(m)][GET_DST_SQR(m)] =
      min(history[player][GET_SRC_SQR(m)][GET_DST_SQR(m)], 100000000 - 1);

  if (history[player][GET_SRC_SQR(m)][GET_DST_SQR(m)] == 100000000 - 1) {
    historyoverflow = 1;
  }
}

void normalizehistory(void) {
  for (int p = 0; p < 2; p++) {
    for (int x = 0; x < 64; x++) {
      for (int y = 0; y < 64; y++) {
        history[p][x][y] /= 2;
      }
    }
  }
  historyoverflow = 0;
}

void clearhistory(void) { memset(history, 0, 2 * 64 * 64 * sizeof(int)); }
