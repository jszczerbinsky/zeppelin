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

#ifndef SEARCH_H
#define SEARCH_H

#include <time.h>

#include "../core/movelist.h"

struct Search_;

typedef struct {
  long timelimit;
  int startdepth;
  int depthlimit;
  int nodeslimit;
  MoveList specificmoves;
} SearchSettings;

typedef struct {
  void (*on_finish)(const struct Search_ *si);
  void (*on_rootmove)(const struct Search_ *si, int score);
  void (*on_nonrootmove)(const struct Search_ *si, int score);
  void (*on_iterfinish)(const struct Search_ *si, int score);
} SearchCallbacks;

typedef struct Search_ {
  int searchid;

  int rootmove_n;
  char rootmove_str[6];
  int root_nodetype;
  int root_repetitions;

  MoveList currline;

  long search_visitednodes;

  clock_t nps_lastcalc;
  long nps_lastnodes;

  int iter_score;
  int iter_depth;
  long iter_visited_nodes;
  Move iter_bestmove;
  int iter_tbhits;
  int iter_highest_depth;

  int prev_iter_score;
  MoveList prev_iter_pv;

  int finished;

  SearchCallbacks cbs;
  SearchSettings set;
} Search;

#define TIME_FOREVER -1

#define DEPTH_INF 99999

#define NODES_INF 0

extern _Atomic int g_abort_search;

void reset_hashtables(void);
void search(const SearchSettings *ss, const SearchCallbacks *cbs);
void stop(void);
int calcnps(void);

static inline void cb_on_finish(const struct Search_ *s) {
  s->cbs.on_finish ? (*s->cbs.on_finish)(s) : (void)0;
}
static inline void cb_on_rootmove(const struct Search_ *s, int score) {
  s->cbs.on_rootmove ? (*s->cbs.on_rootmove)(s, score) : (void)0;
}
static inline void cb_on_nonrootmove(const struct Search_ *s, int score) {
  s->cbs.on_nonrootmove ? (*s->cbs.on_nonrootmove)(s, score) : (void)0;
}
static inline void cb_on_iterfinish(const struct Search_ *s, int score) {
  s->cbs.on_iterfinish ? (*s->cbs.on_iterfinish)(s, score) : (void)0;
}

#endif
