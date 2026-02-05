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

#ifndef GAME_H
#define GAME_H

#include "../eval/nnue.h"
#include "bitboard.h"
#include "movelist.h"

#define WHITE 0
#define BLACK 1
#define ANY 2

#define GAME_F_CANCASTLE_WK 1U
#define GAME_F_CANCASTLE_WQ 2U
#define GAME_F_CANCASTLE_BK 4U
#define GAME_F_CANCASTLE_BQ 8U

#define GAME_F_DEFAULT                                                         \
  (GAME_F_CANCASTLE_WK | GAME_F_CANCASTLE_WQ | GAME_F_CANCASTLE_BK |           \
   GAME_F_CANCASTLE_BQ)

#define CANCASTLE_WK(s) ((s)->flags & GAME_F_CANCASTLE_WK)
#define CANCASTLE_WQ(s) ((s)->flags & GAME_F_CANCASTLE_WQ)
#define CANCASTLE_BK(s) ((s)->flags & GAME_F_CANCASTLE_BK)
#define CANCASTLE_BQ(s) ((s)->flags & GAME_F_CANCASTLE_BQ)

#define FEN_STR_MAX 59

#define PHASE_OPENING 0
#define PHASE_MIDDLEGAME 1
#define PHASE_ENDGAME 2

typedef struct {
  int halfmove;
  int fullmove;
  uint8_t flags;
  BitBrd epbbrd;
  BitBrd hash;
  int phase;
  int nnue_ready;
  int32_t nnue_eval;
} GameState;

typedef struct {
  int who2move;

  BitBrd pieces[3][PIECE_MAX];
  BitBrd piecesof[3];

  MoveList movelist;
  GameState brdstate[MAX_PLY_PER_GAME];
  NNUE nnue;
} Game;

extern Game g_game;
extern GameState *g_gamestate;

int getpieceat(int color, BitBrd bbrd);
void reset_game(void);
char *parsefen(char *fen);
void makemove(Move move);
void unmakemove(void);
void move2str(char *buff, Move move);
Move parsemove(const char *str);
int getrepetitions(void);

static inline int possible_zugzwang(void) {
  return (g_game.piecesof[ANY] & ~g_game.pieces[ANY][PAWN] &
          ~g_game.pieces[ANY][KING]) > 0ULL;
}

#endif
