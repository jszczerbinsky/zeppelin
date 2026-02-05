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

#ifndef EVAL_H
#define EVAL_H

static const int pawnval = 100;
static const int knightval = 300;
static const int bishopval = 310;
static const int rookval = 500;
static const int queenval = 800;

static const int material[] = {pawnval,   0,       knightval,
                               bishopval, rookval, queenval};

#define SCORE_CHECKMATE 99999999
#define SCORE_CHECKMATE_BOUND (SCORE_CHECKMATE - 256)

#define SCORE_CHECKMATED (-SCORE_CHECKMATE)
#define SCORE_ILLEGAL (SCORE_CHECKMATED - 1)

#define IS_CHECKMATE(score)                                                    \
  (((score) >= SCORE_CHECKMATE_BOUND && (score) <= SCORE_CHECKMATE) ||         \
   ((score) <= -SCORE_CHECKMATE_BOUND && (score) >= -SCORE_CHECKMATE))

int evaluate(void);
int evaluate_terminalpos(int pliescnt);
int evaluate_material(void);

void save_eval_entry(int eval);
void dump_eval_entries(int game_result);

#endif
