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

void ttinit(void);
void ttfree(void);

#define KILLER_MAX 5

typedef struct {
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
  Move iter_killers[MAX_PLY_PER_GAME][KILLER_MAX];

  int prev_iter_score;
  MoveList prev_iter_pv;

  int finished;
} SearchInfo;

typedef struct {
  long timelimit;
  int startdepth;
  int depthlimit;
  int nodeslimit;
  MoveList specificmoves;
  void (*on_finish)(const SearchInfo *si);
  void (*on_rootmove)(const SearchInfo *si, size_t ttused, size_t ttsize,
                      int score);
  void (*on_nonrootmove)(const SearchInfo *si, size_t ttused, size_t ttsize,
                         int score);
  void (*on_iterfinish)(const SearchInfo *si, size_t ttused, size_t ttsize,
                        int score);
} SearchSettings;

#define TIME_FOREVER -1

#define DEPTH_INF 99999

#define NODES_INF 0

void reset_hashtables(void);
void search(const SearchSettings *ss);
void stop(void);
int calcnps(void);

#endif
