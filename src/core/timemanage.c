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

#include "game.h"

long getsearchtime(long wtime, long btime, long winc __attribute__((unused)),
                   long binc __attribute__((unused))) {
  long ptime;

  if (g_game.who2move == WHITE) {
    ptime = wtime;
    // pinc = winc;
  } else {
    ptime = btime;
    // pinc = binc;
  }

  int predictedlen;
  if (g_gamestate->fullmove <= 30) {
    predictedlen = 40;
  } else {
    predictedlen = g_gamestate->fullmove + 10;
  }

  return ptime / (predictedlen - g_gamestate->fullmove);
}
