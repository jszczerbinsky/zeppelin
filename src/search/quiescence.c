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

#include "quiescence.h"
#include "../core/movegen.h"
#include "../eval/eval.h"
#include "../settings.h"
#include "../utils/cmputils.h"
#include "order.h"

int quiescence(Search *s, int alpha, int beta) {
  if (g_abort_search) {
    return SCORE_ILLEGAL;
  }

  MoveList availmoves;
  BitBrd attackbbrd;
  gen_moves(g_game.who2move, &availmoves, &attackbbrd, GEN_ALL, 0);

  int standpat;
  if (availmoves.cnt == 0) {
    standpat = evaluate_terminalpos(s->currline.cnt);
  } else {
    standpat = evaluate();
  }

  if (!g_set.disbl_ab && standpat >= beta) {
    return beta;
  }

  int deltaallowed = !g_set.disbl_delta && g_gamestate->phase != PHASE_ENDGAME;

  if (deltaallowed) {
    int delta = material[QUEEN];
    if (standpat < alpha - delta) {
      return alpha;
    }
  }

  s->iter_visited_nodes++;
  s->search_visitednodes++;

  alpha = max(standpat, alpha);

  for (int i = 0; i < availmoves.cnt; i++) {
    order(s, &availmoves, i, NULLMOVE, 0);
    Move currmove = availmoves.move[i];

    if (!IS_CAPT(currmove)) {
      continue;
    }

    pushmove(&s->currline, currmove);
    makemove(currmove);

    if (lastmovelegal()) {
      int score = -quiescence(s, -beta, -alpha);
      unmakemove();
      popmove(&s->currline);

      if (!g_set.disbl_ab && score >= beta) {
        return beta;
      }
      alpha = max(score, alpha);
    } else {
      unmakemove();
      popmove(&s->currline);
    }
  }
  return alpha;
}
