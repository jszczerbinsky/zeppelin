#include <stdio.h>

#include "main.h"

// every 3 rows: opening, middlegame, endgame
int eval_weights[PATTERNS_SIZE * 3] = {0};

static BitBrd _patterns[PATTERNS_SIZE];

static const BitBrd FILES[] = {FILE_A, FILE_B, FILE_C, FILE_D,
                               FILE_E, FILE_F, FILE_G, FILE_H};

static const BitBrd RANKS[] = {RANK_1, RANK_2, RANK_3, RANK_4,
                               RANK_5, RANK_6, RANK_7, RANK_8};

static const BitBrd PAWN_COVER_KSIDE_PAWNS[] = {
    0xe000ULL, 0x20c000ULL, 0x40a000ULL, 0x806000ULL, 0x608000ULL, 0xc02000ULL,
};

static const BitBrd PAWN_COVER_KSIDE_KING[] = {0x20ULL, 0x40ULL, 0x80ULL};

static const BitBrd PAWN_COVER_QSIDE_PAWNS[] = {
    0x700ULL, 0x10600ULL, 0x20500ULL, 0x40300ULL, 0x30400ULL, 0x60100ULL,
};

static const BitBrd PAWN_COVER_QSIDE_KING[] = {0x1ULL, 0x2ULL, 0x4ULL};

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

static int _eval_side(const EvalSideArgs *args) {
  // From white perspective

  BitBrd rookqueens = args->ppieces[ROOK] | args->ppieces[QUEEN];

  int kingsqr = bbrd2sqr(args->ppieces[KING]);

  int x = 0;

  // piece-square pairs [384]
  for (int p = 0; p < PIECE_MAX; p++) {
    for (int sqr = 0; sqr < 64; sqr++) {
      _patterns[x++] = sqr2bbrd(sqr) & args->ppieces[p];
    }
  }

  // knight horizontal span [4]
  _patterns[x++] = (FILE_A | FILE_H) & args->ppieces[KNIGHT];
  _patterns[x++] = (FILE_B | FILE_G) & args->ppieces[KNIGHT];
  _patterns[x++] = (FILE_C | FILE_F) & args->ppieces[KNIGHT];
  _patterns[x++] = (FILE_D | FILE_E) & args->ppieces[KNIGHT];

  // pawn horizontal span [4]
  _patterns[x++] = (FILE_A | FILE_H) & args->ppieces[PAWN];
  _patterns[x++] = (FILE_B | FILE_G) & args->ppieces[PAWN];
  _patterns[x++] = (FILE_C | FILE_F) & args->ppieces[PAWN];
  _patterns[x++] = (FILE_D | FILE_E) & args->ppieces[PAWN];

  // pawn advancement [8]
  for (int i = 0; i < 8; i++)
    _patterns[x++] = RANKS[i] & args->ppieces[PAWN];

  // center [2]
  _patterns[x++] = args->pallpieces & CENTER;
  _patterns[x++] = args->pallpieces & CENTER16;

  // pawns in center [2]
  _patterns[x++] = args->ppieces[PAWN] & CENTER;
  _patterns[x++] = args->ppieces[PAWN] & CENTER16;

  // king in center [2]
  _patterns[x++] = args->ppieces[KING] & CENTER;
  _patterns[x++] = args->ppieces[KING] & CENTER16;

  // king advancement [8]
  for (int i = 0; i < 8; i++)
    _patterns[x++] = RANKS[i] & args->ppieces[KING];

  // king horizontal span [4]
  _patterns[x++] = (FILE_A | FILE_H) & args->ppieces[KING];
  _patterns[x++] = (FILE_B | FILE_G) & args->ppieces[KING];
  _patterns[x++] = (FILE_C | FILE_F) & args->ppieces[KING];
  _patterns[x++] = (FILE_D | FILE_E) & args->ppieces[KING];

  // pieces near king [2]
  _patterns[x++] = nearby(args->ppieces[KING]) & args->pallpieces;
  _patterns[x++] = nearby(args->ppieces[KING]) & args->eallpieces;

  // doubled pawns [1]
  _patterns[x++] = args->ppieces[PAWN] &
                   (args->ppieces[PAWN] >> 8 | args->ppieces[PAWN] >> 16 |
                    args->ppieces[PAWN] >> 24 | args->ppieces[PAWN] >> 32 |
                    args->ppieces[PAWN] >> 40 | args->ppieces[PAWN] >> 48 |
                    args->ppieces[PAWN] >> 56);

  // doubled rooks/queens [1]
  _patterns[x++] =
      rookqueens & (rookqueens >> 8 | rookqueens >> 16 | rookqueens >> 24 |
                    rookqueens >> 32 | rookqueens >> 40 | rookqueens >> 48 |
                    rookqueens >> 56);

  // king on the same file with a rook/queen [1]
  _patterns[x++] = rookqueens & FILES[kingsqr % 8];

  // king castled [1]
  _patterns[x++] = args->ppieces[KING] &
                   (sqr2bbrd(CASTLE_WK_KINGSQR) | sqr2bbrd(CASTLE_WQ_KINGSQR));

  // undeveloped light pieces [1]
  _patterns[x++] = (args->ppieces[BISHOP] | args->ppieces[KNIGHT]) & RANK_1;

  // king decastled [1]
  _patterns[x++] = args->ppieces[KING] & 0x3828ULL;

  // pawn cover kingside [18]
  for (int k = 0; k < 3; k++) {
    for (int p = 0; p < 6; p++) {
      int match = (args->ppieces[PAWN] & PAWN_COVER_KSIDE_PAWNS[p]) ==
                  PAWN_COVER_KSIDE_PAWNS[p];
      match &= (args->ppieces[KING] & PAWN_COVER_KSIDE_KING[k]) ==
               PAWN_COVER_KSIDE_KING[k];

      _patterns[x++] = match ? 1ULL : 0ULL;
    }
  }

  // pawn cover queenside [18]
  for (int k = 0; k < 3; k++) {
    for (int p = 0; p < 6; p++) {
      int match = (args->ppieces[PAWN] & PAWN_COVER_QSIDE_PAWNS[p]) ==
                  PAWN_COVER_QSIDE_PAWNS[p];
      match &= (args->ppieces[KING] & PAWN_COVER_QSIDE_KING[k]) ==
               PAWN_COVER_QSIDE_KING[k];

      _patterns[x++] = match ? 1ULL : 0ULL;
    }
  }

  // mobility [1]
  _patterns[x++] = args->pattacks;

  // attacked pieces [1]
  _patterns[x++] = args->pattacks & args->eallpieces;

  // defended pieces under attack [1]
  _patterns[x++] = args->pattacks & args->pallpieces & args->eattacks;

  // defended pieces not under attack [1]
  _patterns[x++] = args->pattacks & args->pallpieces & (~args->eattacks);

  // undefended pieces under attack [1]
  _patterns[x++] = (~args->pattacks) & args->pallpieces & args->eattacks;

  // undefended pieces not under attack [1]
  _patterns[x++] = (~args->pattacks) & args->pallpieces & (~args->eattacks);

  int offset;
  switch (g_gamestate->phase) {
  case PHASE_OPENING:
    offset = 0;
  case PHASE_MIDDLEGAME:
    offset = 1;
  case PHASE_ENDGAME:
    offset = 2;
  }

  int sum = 0;
  for (int i = 0; i < PATTERNS_SIZE; i++) {
    sum += popcnt(_patterns[i]) * eval_weights[3 * i + offset];
  }

  return sum;
}

int evaluate(int pliescnt) {
  int value = 0;

  BitBrd wattacks, battacks;
  MoveList wmoves, bmoves;
  gen_moves(WHITE, &wmoves, &wattacks, GEN_ALL, 0);
  gen_moves(BLACK, &bmoves, &battacks, GEN_ALL, 0);

  BitBrd bflipped_piece[PIECE_MAX];
  BitBrd bflipped;
  BitBrd wflipped_piece[PIECE_MAX];
  BitBrd wflipped;
  BitBrd wattacks_flipped = bbrdflipv(wattacks);
  BitBrd battacks_flipped = bbrdflipv(battacks);
  bflipped = bbrdflipv(g_game.piecesof[BLACK]);
  wflipped = bbrdflipv(g_game.piecesof[WHITE]);
  for (int i = 0; i < PIECE_MAX; i++) {
    bflipped_piece[i] = bbrdflipv(g_game.pieces[BLACK][i]);
    wflipped_piece[i] = bbrdflipv(g_game.pieces[WHITE][i]);
  }

  EvalSideArgs wargs = {
      .pcolor = WHITE,
      .ppieces = g_game.pieces[WHITE],
      .pallpieces = g_game.piecesof[WHITE],
      .pattacks = wattacks,
      .epieces = g_game.pieces[BLACK],
      .eallpieces = g_game.piecesof[BLACK],
      .eattacks = battacks,
  };
  EvalSideArgs bargs = {
      .pcolor = BLACK,
      .ppieces = bflipped_piece,
      .pallpieces = bflipped,
      .pattacks = battacks_flipped,
      .epieces = wflipped_piece,
      .eallpieces = wflipped,
      .eattacks = wattacks_flipped,
  };

  value += _eval_side(&wargs) - _eval_side(&bargs);

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

  fread(eval_weights, sizeof(int), PATTERNS_SIZE * 3, in);

  fclose(in);

  return 1;
}
