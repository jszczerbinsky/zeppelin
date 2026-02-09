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

#include <limits.h>

#include "../core/game.h"
#include "../core/movegen.h"
#include "../eval/eval.h"
#include "../settings.h"
#include "history.h"
#include "killer.h"
#include "order.h"
#include "search.h"

static int see(int sqr) {
  MoveList capts;
  BitBrd attackbbrd;
  gen_moves(g_game.who2move, &capts, &attackbbrd, GEN_CAPT, 0);

  Move lva = NULLMOVE;
  for (int i = 0; i < capts.cnt; i++) {
    const Move currmove = capts.move[i];

    if (GET_DST_SQR(currmove) == sqr) {
      if (lva == NULLMOVE ||
          material[GET_MOV_PIECE(currmove)] < material[GET_MOV_PIECE(lva)]) {
        makemove(currmove);
        if (lastmovelegal()) {
          lva = currmove;
        }
        unmakemove();
      }
    }
  }

  if (lva == NULLMOVE) {
    return 0;
  }

  makemove(lva);
  int val = material[GET_CAPT_PIECE(lva)] - see(sqr);
  if (IS_PROM(lva)) {
    val += material[GET_PROM_PIECE(lva)] - 1;
  }
  unmakemove();

  return val > 0 ? val : 0;
}

static int get_priority(const Search *s, Move move, Move ttbest, int ispv) {

  const int pvpriority = INT_MAX;
  const int ttpriority = INT_MAX - 1;
  const int captpriority = INT_MAX - 10000;
  const int killerpriority = INT_MAX - 20001;
  const int normalpriority = 0;

  // ordering before making move, thus ply=cnt
  int ply = s->currline.cnt;

  if (ispv && ply < s->prev_iter_pv.cnt && move == s->prev_iter_pv.move[ply]) {
    return pvpriority;
  }

  if (move == ttbest) {
    return ttpriority;
  }

  if (!g_set.disbl_killer && iskiller(ply, move)) {
    return killerpriority;
  }

  int diff = 0;
  if (IS_CAPT(move)) {
    int sqr = GET_DST_SQR(move);
    diff = 0;
    makemove(move);
    if (lastmovelegal()) {
      diff = material[GET_CAPT_PIECE(move)] - see(sqr);
      if (IS_PROM(move)) {
        diff += material[GET_PROM_PIECE(move)] - 1;
      }
      unmakemove();
    } else {

      unmakemove();
      return INT_MIN;
    }
    return captpriority + diff;
  }

  if (IS_PROM(move)) {
    diff += material[GET_PROM_PIECE(move)];
    return captpriority + diff;
  }

  if (IS_CASTLE(move))
    return normalpriority;

  return normalpriority +
         history[g_game.who2move][GET_SRC_SQR(move)][GET_DST_SQR(move)];
}

int order(const Search *s, MoveList *movelist, int curr, Move ttbest,
          int ispv) {
  int best_i = -1;
  int best_priority = INT_MIN;

  for (int i = curr; i < movelist->cnt; i++) {
    int priority = get_priority(s, movelist->move[i], ttbest, ispv);
    if (priority > best_priority) {
      best_i = i;
      best_priority = priority;
    }
  }
  Move best = movelist->move[best_i];
  movelist->move[best_i] = movelist->move[curr];
  movelist->move[curr] = best;

  return best_priority != INT_MIN;
}
