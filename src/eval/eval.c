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

#include <stdio.h>
#include <stdlib.h>

#include "../core/game.h"
#include "../core/movegen.h"
#include "../settings.h"
#include "eval.h"

typedef struct {
  int pcolor;

  // player
  const BitBrd *ppieces;
  const BitBrd pallpieces;
  const BitBrd pattacks;

  // enemy
  const BitBrd *epieces;
  const BitBrd eallpieces;
  const BitBrd eattacks;
} EvalSideArgs;

int evaluate(void) {
#ifdef EVAL_NNUE
  if (!g_gamestate->nnue_ready)
    nnue_calc_deep_acc(&g_game.nnue);

  // nnue_init(&g_game.nnue);
  if (g_game.who2move == WHITE)
    // return (int)g_game.nnue.out;
    return (int)g_gamestate->nnue_eval;
  else
    // return -(int)g_game.nnue.out;
    return -(int)g_gamestate->nnue_eval;
#endif

#ifdef EVAL_MATERIAL_ONLY
  int value = 0;
  for (int p = 0; p < PIECE_MAX; p++) {
    value +=
        (popcnt(g_game.pieces[WHITE][p]) - popcnt(g_game.pieces[BLACK][p])) *
        material[p];
  }
  return g_game.who2move == WHITE ? value : -value;
#endif
}

int evaluate_terminalpos(int pliescnt) {
  if (get_under_check_cnt() > 0) {
    return SCORE_CHECKMATED + pliescnt;
  } else {
    return 0;
  }
}

int evaluate_material(void) {
  int value = 0;
  for (int p = 0; p < PIECE_MAX; p++) {
    value += (popcnt(g_game.pieces[g_game.who2move][p]) -
              popcnt(g_game.pieces[!g_game.who2move][p])) *
             material[p];
  }
  return value;
}

typedef struct {
  BitBrd occupancy;
  int piecescnt;
  uint8_t *piece_pairs;
  int pairscnt;
  int32_t eval;
  uint8_t fullmove;
} EvalEntry;

static int eval_entries_cnt = 0;
static EvalEntry *eval_entries = NULL;

void save_eval_entry(int eval) {
  eval_entries_cnt++;
  eval_entries =
      realloc(eval_entries, (size_t)eval_entries_cnt * sizeof(EvalEntry));

  EvalEntry *new_entry = eval_entries + (eval_entries_cnt - 1);
  new_entry->occupancy = g_game.piecesof[ANY];
  new_entry->fullmove = (uint8_t)g_gamestate->fullmove;

  new_entry->piecescnt = popcnt(new_entry->occupancy);
  new_entry->pairscnt = (new_entry->piecescnt + 1) / 2;

  new_entry->piece_pairs = calloc((size_t)new_entry->pairscnt, sizeof(uint8_t));

  int pair = 0;
  int ishi = 1;
  for (int sqr = 0; sqr < 64; sqr++) {
    int piece = getpieceat(ANY, sqr2bbrd(sqr));
    int isblack = getpieceat(BLACK, sqr2bbrd(sqr)) != -1;

    if (piece != -1) {
      uint8_t fpiece = (uint8_t)piece;
      if (isblack)
        fpiece |= 8U;

      if (ishi) {
        fpiece = (uint8_t)(fpiece << 4);
        new_entry->piece_pairs[pair] = fpiece;
      } else {
        new_entry->piece_pairs[pair] |= fpiece;
      }

      if (!ishi)
        pair++;
      ishi = !ishi;
    }
  }
  new_entry->eval = (int32_t)eval;
}

void dump_eval_entries(int game_result) {
  int8_t fgame_result = (int8_t)game_result;
  uint8_t total_fullmoves = (uint8_t)g_gamestate->fullmove;

  FILE *f = fopen("dataset", "r+b");
  if (!f) {
    f = fopen("dataset", "w+b");
  }

  fseek(f, 0, SEEK_END);
  ssize_t fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint32_t count = 0;
  if (fsize >= 4)
    fread(&count, sizeof(uint32_t), 1, f);
  count += (uint32_t)eval_entries_cnt;
  fseek(f, 0, SEEK_SET);
  fwrite(&count, sizeof(uint32_t), 1, f);

  fseek(f, 0, SEEK_END);

  for (int i = 0; i < eval_entries_cnt; i++) {
    fwrite(&eval_entries[i].occupancy, sizeof(BitBrd), 1, f);
    fwrite(eval_entries[i].piece_pairs, sizeof(uint8_t),
           (size_t)eval_entries[i].pairscnt, f);
    fwrite(&eval_entries[i].eval, sizeof(int32_t), 1, f);
    fwrite(&fgame_result, sizeof(int8_t), 1, f);
    fwrite(&eval_entries[i].fullmove, sizeof(uint8_t), 1, f);
    fwrite(&total_fullmoves, sizeof(uint8_t), 1, f);

    free(eval_entries[i].piece_pairs);
  }

  fclose(f);

  free(eval_entries);
  eval_entries = NULL;

  g_set.gen_evals = 0;
}
