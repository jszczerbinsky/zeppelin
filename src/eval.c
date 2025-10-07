#include <stdio.h>

#include "main.h"

int eval_weights[PATTERNS_SIZE] = {0};

static BitBrd _patterns[PATTERNS_SIZE];

static const BitBrd FILES[] = {FILE_A, FILE_B, FILE_C, FILE_D,
                               FILE_E, FILE_F, FILE_G, FILE_H};

static const BitBrd RANKS[] = {RANK_1, RANK_2, RANK_3, RANK_4,
                               RANK_5, RANK_6, RANK_7, RANK_8};

static int _eval_structures(int color, const BitBrd pieces[PIECE_MAX],
                            BitBrd allpieces, const BitBrd epieces[PIECE_MAX],
                            BitBrd eallpieces) {
  // From white perspective

  BitBrd rookqueens = pieces[ROOK] | pieces[QUEEN];

  int kingsqr = bbrd2sqr(pieces[KING]);

  int x = 0;

  // piece-square pairs [384]
  for (int p = 0; p < PIECE_MAX; p++) {
    for (int sqr = 0; sqr < 64; sqr++) {
      _patterns[x++] = sqr2bbrd(sqr) & pieces[p];
    }
  }

  // knight horizontal span [4]
  _patterns[x++] = (FILE_A | FILE_H) & pieces[KNIGHT];
  _patterns[x++] = (FILE_B | FILE_G) & pieces[KNIGHT];
  _patterns[x++] = (FILE_C | FILE_F) & pieces[KNIGHT];
  _patterns[x++] = (FILE_D | FILE_E) & pieces[KNIGHT];

  // pawn horizontal span [4]
  _patterns[x++] = (FILE_A | FILE_H) & pieces[PAWN];
  _patterns[x++] = (FILE_B | FILE_G) & pieces[PAWN];
  _patterns[x++] = (FILE_C | FILE_F) & pieces[PAWN];
  _patterns[x++] = (FILE_D | FILE_E) & pieces[PAWN];

  // pawn advancement [8]
  for (int i = 0; i < 8; i++)
    _patterns[x++] = RANKS[i] & pieces[PAWN];

  // center [2]
  _patterns[x++] = allpieces & CENTER;
  _patterns[x++] = allpieces & CENTER16;

  // pawns in center [2]
  _patterns[x++] = pieces[PAWN] & CENTER;
  _patterns[x++] = pieces[PAWN] & CENTER16;

  // king in center [2]
  _patterns[x++] = pieces[KING] & CENTER;
  _patterns[x++] = pieces[KING] & CENTER16;

  // king advancement [8]
  for (int i = 0; i < 8; i++)
    _patterns[x++] = RANKS[i] & pieces[KING];

  // king horizontal span [4]
  _patterns[x++] = (FILE_A | FILE_H) & pieces[KING];
  _patterns[x++] = (FILE_B | FILE_G) & pieces[KING];
  _patterns[x++] = (FILE_C | FILE_F) & pieces[KING];
  _patterns[x++] = (FILE_D | FILE_E) & pieces[KING];

  // pieces near king [2]
  _patterns[x++] = nearby(pieces[KING]) & allpieces;
  _patterns[x++] = nearby(pieces[KING]) & eallpieces;

  // doubled pawns [1]
  _patterns[x++] = pieces[PAWN] & (pieces[PAWN] >> 8 | pieces[PAWN] >> 16 |
                                   pieces[PAWN] >> 24 | pieces[PAWN] >> 32 |
                                   pieces[PAWN] >> 40 | pieces[PAWN] >> 48 |
                                   pieces[PAWN] >> 56);

  // doubled rooks/queens [1]
  _patterns[x++] =
      rookqueens & (rookqueens >> 8 | rookqueens >> 16 | rookqueens >> 24 |
                    rookqueens >> 32 | rookqueens >> 40 | rookqueens >> 48 |
                    rookqueens >> 56);

  // king on the same file with a rook/queen [1]
  _patterns[x++] = rookqueens & FILES[kingsqr % 8];

  // king castled [1]
  _patterns[x++] = pieces[KING] &
                   (sqr2bbrd(CASTLE_WK_KINGSQR) | sqr2bbrd(CASTLE_WQ_KINGSQR));

  // undeveloped pieces [1]
  _patterns[x++] = allpieces & (~pieces[KING]) & RANK_1;

  // king decastled [1]
  _patterns[x++] = pieces[KING] & 0x3828ULL;

  int sum = 0;
  for (int i = 0; i < PATTERNS_SIZE; i++) {
    sum += popcnt(_patterns[i]) * eval_weights[i];
  }

  return sum;
}

int evaluate(int pliescnt) {
  int value = 0;

  BitBrd flipped_piece[PIECE_MAX];
  BitBrd flipped;
  BitBrd eflipped_piece[PIECE_MAX];
  BitBrd eflipped;
  flipped = bbrdflipv(g_game.piecesof[BLACK]);
  eflipped = bbrdflipv(g_game.piecesof[WHITE]);
  for (int i = 0; i < PIECE_MAX; i++) {
    flipped_piece[i] = bbrdflipv(g_game.pieces[BLACK][i]);
    eflipped_piece[i] = bbrdflipv(g_game.pieces[WHITE][i]);
  }

  value += _eval_structures(g_game.who2move, g_game.pieces[WHITE],
                            g_game.piecesof[WHITE], g_game.pieces[BLACK],
                            g_game.piecesof[BLACK]) -
           _eval_structures(!g_game.who2move, flipped_piece, flipped,
                            eflipped_piece, eflipped);

  for (int p = 0; p < PIECE_MAX; p++) {
    value +=
        (popcnt(g_game.pieces[WHITE][p]) - popcnt(g_game.pieces[BLACK][p])) *
        material[p];
  }

  return g_game.who2move == WHITE ? value : -value;
}

int evaluate_terminalpos(int pliescnt) {
  if (get_under_check_cnt() > 0) {
    return SCORE_CHECKMATED + pliescnt;
  } else {
    return 0;
  }
}

int loadweights() {
  FILE *in = fopen("weights.bin", "rb");
  if (!in)
    return 0;

  fread(eval_weights, sizeof(int), PATTERNS_SIZE, in);

  fclose(in);

  return 1;
}
