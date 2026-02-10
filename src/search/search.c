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
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "../core/game.h"
#include "../eval/eval.h"
#include "../iface/iface.h"
#include "../settings.h"
#include "../utils/timeutils.h"
#include "history.h"
#include "killer.h"
#include "negamax.h"
#include "search.h"

static Search si;

static pthread_t search_thread;
static pthread_t supervisor_thread;

static _Atomic int manualstop = 0;
_Atomic int g_abort_search = 0;

static int searchid = 0;

int calcnps(void) {
  clock_t now = clock();

  double seconds = (double)(now - si.nps_lastcalc) / (double)CLOCKS_PER_SEC;
  long nodesdiff = si.iter_visited_nodes; //- si.nps_lastnodes;
                                          //
  if (seconds < 0.001 || nodesdiff < 1) {
    return 0;
  }

  int nps = (int)((double)nodesdiff / seconds);

  // si.nps_lastnodes = si.search_visitednodes;
  // si.nps_lastcalc = clock();

  return nps;
}

void reset_hashtables(void) {}
static void search_finish(void) {
  if (si.prev_iter_pv.cnt > 0) {
    cb_on_finish(&si);
  } else {
    PRINTDBG("no legal move detected");
    fflush(stdout);
  }
  si.finished = 1;
}

static void *search_subthread(void *arg __attribute__((unused))) {
  int firsttime = 1;
  int lastscore = SCORE_ILLEGAL;

  clearhistory();
  si.search_visitednodes = 0;
  for (int depth = si.set.startdepth; depth <= si.set.depthlimit; depth++) {
    searchid++;
    si.searchid = searchid;
    si.root_repetitions = getrepetitions();
    si.iter_highest_depth = 0;
    si.currline.cnt = 0;
    si.iter_tbhits = 0;
    si.iter_visited_nodes = 0;
    si.nps_lastnodes = 0;
    si.nps_lastcalc = clock();
    si.rootmove_n = 0;
    si.iter_depth = depth;
    clearkiller();

    if (historyoverflow) {
      normalizehistory();
    }

    MoveList pv = {0};
    int score;
    if (g_set.disbl_aspwnd || firsttime || IS_CHECKMATE(lastscore)) {
      score = negamax(&si, SCORE_ILLEGAL, -SCORE_ILLEGAL, depth, &pv, 1);
      firsttime = 0;

      if (g_abort_search) {
        return 0;
      }
    } else {
      int inbounds = 0;
      const int sizes[] = {25, 50, 100, 200, 400};
      const int sizes_max = sizeof(sizes) / sizeof(int);

      int asize_i = 0;
      int bsize_i = 0;
      do {
        int alpha;
        int beta;

        if (asize_i < sizes_max) {
          alpha = lastscore - sizes[asize_i];
        } else {
          alpha = SCORE_ILLEGAL;
        }
        if (bsize_i < sizes_max) {
          beta = lastscore + sizes[bsize_i];
        } else {
          beta = -SCORE_ILLEGAL;
        }

        score = negamax(&si, alpha, beta, depth, &pv, 1);

        if (g_abort_search) {
          return 0;
        }

        if (si.root_nodetype == NODE_FAILL) {
          asize_i++;
        } else if (si.root_nodetype == NODE_FAILH) {
          bsize_i++;
        } else {
          inbounds = 1;
        }
      } while (!inbounds);
    }

    si.prev_iter_score = lastscore;
    si.iter_score = score;
    lastscore = score;
    // recoverpv(&pv);

    memcpy(&si.prev_iter_pv, &pv, sizeof(MoveList));
    // ON_ITERFINISH(score);
    cb_on_iterfinish(&si, score);

    if (IS_CHECKMATE(score)) {
      break;
    }
  }

  si.finished = 1;
  return 0;
}

static void *supervisor_subthread(void *arg __attribute__((unused))) {
  pthread_create(&search_thread, NULL, search_subthread, NULL);

  while (si.ispondering) {
    usleep(1000);
  }

  clock_t start_time = clock() / CLOCKS_PER_MS;
  long start_iter_nodes = si.iter_visited_nodes;

  while (1) {
    if (si.finished) {
      break;
    }
    // todo PRINTDBG needs lock
    if (si.set.timelimit != TIME_FOREVER &&
        difftime(clock() / CLOCKS_PER_MS, start_time) >=
            (double)si.set.timelimit) {
      // PRINTDBG("cancelling on time");
      break;
    }
    if (si.set.nodeslimit != 0 &&
        si.iter_visited_nodes - start_iter_nodes >= si.set.nodeslimit) {
      // PRINTDBG("canceling on nodes limit");
      break;
    }
    if (manualstop) {
      // PRINTDBG("canceling on time");
      break;
    }

    usleep(1000);
  }
  fflush(stdout);

  while (si.prev_iter_pv.cnt == 0) {
    usleep(1000);
  }

  // pthread_cancel(search_thread);
  g_abort_search = 1;
  pthread_join(search_thread, NULL);

  search_finish();
  return 0;
}

void ponderhit(void) { si.ispondering = 0; }

void search(const SearchSettings *settings, const SearchCallbacks *callbacks,
            int ponder) {
  memset(&si, 0, sizeof(Search));

  memcpy(&si.set, settings, sizeof(SearchSettings));
  memcpy(&si.cbs, callbacks, sizeof(SearchCallbacks));

  manualstop = 0;
  g_abort_search = 0;
  si.finished = 0;
  si.ispondering = ponder;
  pthread_create(&supervisor_thread, NULL, supervisor_subthread, NULL);
}

void stop(void) {
  PRINTDBG("canceling manually");
  fflush(stdout);
  manualstop = 1;
  si.ispondering = 0;
  pthread_join(supervisor_thread, NULL);
}
