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

#include <math.h>

#include "../core/movegen.h"
#include "../eval/eval.h"
#include "../settings.h"
#include "../utils/cmputils.h"
#include "history.h"
#include "killer.h"
#include "negamax.h"
#include "order.h"
#include "quiescence.h"
#include "search.h"
#include "tt.h"

typedef struct {
  MoveList availmoves;
  Move bestmove;
  int nodetype;
  int legalcnt;
  int score;
} NodeInfo;

static void negamax_inner(Search *s, NodeInfo *ni, int depthleft, int *alpha,
                          int beta, Move ttbest, MoveList *pvdest, int ispv) {
  ni->legalcnt = 0;
  ni->nodetype = NODE_FAILL;
  ni->bestmove = NULLMOVE;
  int score = SCORE_ILLEGAL;

  int stat_eval = evaluate();
  int under_check_cnt = get_under_check_cnt();
  Move last_move = s->currline.move[s->currline.cnt - 1];

  if (!g_set.disbl_nmp && g_gamestate->phase != PHASE_ENDGAME &&
      !possible_zugzwang() && under_check_cnt == 0 && s->currline.cnt != 0 &&
      depthleft > 3 && last_move != NULLMOVE && stat_eval >= beta) {
    makemove(NULLMOVE);
    pushmove(&s->currline, NULLMOVE);
    int nmpscore = -negamax(s, -beta, -beta + 1, depthleft - 1 - 2, NULL, 0);
    popmove(&s->currline);
    unmakemove();

    if (nmpscore >= beta) {
      ni->nodetype = NODE_SELECTIVELY_PRUNED;
      ni->score = beta;
      return;
    }
  }

  BitBrd attackbbrd;
  gen_moves(g_game.who2move, &ni->availmoves, &attackbbrd, GEN_ALL,
            under_check_cnt);

  for (int i = 0; i < ni->availmoves.cnt; i++) {
    order(s, &ni->availmoves, i, ttbest, ispv);
    Move currmove = ni->availmoves.move[i];

    if (s->currline.cnt == 0 && s->set.specificmoves.cnt > 0 &&
        !containsmove(&s->set.specificmoves, currmove)) {
      continue;
    }

    pushmove(&s->currline, currmove);
    makemove(currmove);

    if (lastmovelegal()) {
      ni->legalcnt++;

      int subispv = ispv && ni->legalcnt == 1;

      int movescore = 0;
      int fullsearch = 1;
      int new_under_check_cnt = get_under_check_cnt();

      int pvsallowed = !g_set.disbl_pvs && ni->legalcnt > 1;
      int fpallowed = !g_set.disbl_fp && !ispv && depthleft <= 2 &&
                      !IS_CAPT(currmove) && new_under_check_cnt == 0;

      // Extensions
      int ext = 0;
      if (depthleft == 1 && new_under_check_cnt > 0) {
        ext += 1;
      }

      // LMR
      if (!g_set.disbl_lmr && ext == 0 && depthleft > 3 && !IS_CAPT(currmove) &&
          ni->legalcnt > 4) {
        if (IS_SILENT(currmove)) {
          ext -= (int)(0.8 + (log(depthleft) * log(ni->legalcnt - 3) / 2));
        } else {
          ext -= (int)(0.2 + (log(depthleft) * log(ni->legalcnt - 3) / 4));
        }
      }

      // Futility Pruning
      if (fpallowed && ext <= 0) {
        if (-evaluate_material() <
            *alpha - 100 * depthleft + 50 * (depthleft - 1)) {
          movescore = *alpha;
          pvsallowed = 0;
          fullsearch = 0;
        }
      }

      // PVS
      if (pvsallowed) {
        int pvsscore = -negamax(s, -*alpha - 1, -*alpha, depthleft + ext - 1,
                                NULL, subispv);

        if (pvsscore <= *alpha) {
          fullsearch = 0;
          movescore = pvsscore;
        }
      }

      MoveList subpv = {0};

      // Full search if required
      if (fullsearch) {
        movescore =
            -negamax(s, -beta, -*alpha, depthleft + ext - 1, &subpv, subispv);

        if (ext < 0 && movescore > *alpha) {
          // reduced move raised alpha
          // re-search without reductions
          subpv.cnt = 0;
          movescore =
              -negamax(s, -beta, -*alpha, depthleft - 1, &subpv, subispv);
        }
      }

      if (s->currline.cnt == 1) {
        move2str(s->rootmove_str, ni->availmoves.move[i]);
        s->rootmove_n++;
        // ON_ROOTMOVE(movescore);
        cb_on_rootmove(s, movescore);
      } else {
        // ON_NONROOTMOVE(movescore);
        cb_on_nonrootmove(s, movescore);
      }

      score = max(score, movescore);

      if (!g_set.disbl_ab) {
        if (score >= beta) {
          unmakemove();
          popmove(&s->currline);
          if (IS_SILENT(currmove)) {
            if (!g_set.disbl_killer) {
              addkiller(s->currline.cnt, currmove);
            }
            addhistory(g_game.who2move, currmove, 1);
          }

          ni->nodetype = NODE_FAILH;
          ni->score = score;
          return;
        } else {
        }
      }

      if (score > *alpha) {
        ni->nodetype = NODE_INSIDEWND;
        ni->bestmove = currmove;
        if (pvdest != NULL) {
          pvdest->cnt = 1;
          pvdest->move[0] = currmove;
          for (int m = 0; m < subpv.cnt; m++) {
            if (subpv.move[m] == NULLMOVE) {
              break;
            }
            pushmove(pvdest, subpv.move[m]);
          }
        }
        *alpha = score;
      }
    }

    unmakemove();
    popmove(&s->currline);
  }

  if (ni->legalcnt == 0) {
    ni->nodetype = NODE_GAMEFINISHED;
    ni->score = evaluate_terminalpos(s->currline.cnt);
  } else {
    ni->score = score;
  }
}

int negamax(Search *s, int alpha, int beta, int depthleft, MoveList *pvdest,
            int ispv) {
  if (pvdest)
    pvdest->cnt = 0;

  const BitBrd hash = g_gamestate->hash;

  if (g_abort_search) {
    return SCORE_ILLEGAL;
  }

  if (s->root_repetitions >= 3) {
    return 0;
  }

  if (s->currline.cnt > 0) {
    int rep = getrepetitions();
    if (rep > 0)
      return 0;
  }

  if (g_gamestate->halfmove >= 100) {
    return 0;
  }

  Move ttbest = NULLMOVE;
  if (!g_set.disbl_tt && s->currline.cnt > 0) {
    const TT *ttentry = ttread(hash, depthleft, s->searchid);
    if (ttentry) {
      ttbest = ttentry->bestmove;
      s->iter_tbhits++;

      switch (ttentry->type) {
      case TT_EXACT:
        return ttentry->value;
        break;
      case TT_UPPERBOUND:
        beta = min(beta, ttentry->value);
        break;
      case TT_LOWERBOUND:
        alpha = max(alpha, ttentry->value);
        break;
      }
      if (alpha >= beta) {
        return ttentry->value;
      }
    }
  }

  s->iter_visited_nodes++;
  s->search_visitednodes++;

  if (s->currline.cnt > s->iter_highest_depth) {
    s->iter_highest_depth = s->currline.cnt;
  }

  if (depthleft <= 0) {
    if (!g_set.disbl_quiescence) {
      return quiescence(s, alpha, beta);
    } else {
      MoveList availmoves;
      BitBrd attackbbrd;
      gen_moves(g_game.who2move, &availmoves, &attackbbrd, GEN_ALL, 0);

      if (availmoves.cnt == 0) {
        return evaluate_terminalpos(s->currline.cnt);
      } else {
        return evaluate();
      }
    }
  }

  NodeInfo ni;
  negamax_inner(s, &ni, depthleft, &alpha, beta, ttbest, pvdest, ispv);

  if (s->currline.cnt == 0) {
    s->iter_bestmove = ni.bestmove;
    s->root_nodetype = ni.nodetype;
  }

  switch (ni.nodetype) {
  case NODE_FAILL:
    ttwrite(hash, TT_UPPERBOUND, depthleft, alpha, ni.bestmove, s->searchid);
    break;
  case NODE_FAILH:
    ttwrite(hash, TT_LOWERBOUND, depthleft, beta, ni.bestmove, s->searchid);
    break;
  case NODE_INSIDEWND:
  case NODE_GAMEFINISHED:               // Illegal (mated or stalemated)
    if (s->set.specificmoves.cnt > 0) { // possible better move wasnt checked
      ttwrite(hash, TT_LOWERBOUND, depthleft, ni.score, ni.bestmove,
              s->searchid);
    } else {
      ttwrite(hash, TT_EXACT, depthleft, ni.score, ni.bestmove, s->searchid);
    }
    break;
  default:
    break;
  }

  return ni.score;
}
