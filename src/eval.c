#include "main.h"

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

static int _eval_side(const EvalSideArgs *args) { return 0; }

int evaluate() {

  nnue_init(&g_game.nnue);
  if (g_game.who2move == WHITE)
    return (int)g_game.nnue.out;
  else
    return -(int)g_game.nnue.out;

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

int evaluate_material() {
  int value = 0;
  for (int p = 0; p < PIECE_MAX; p++) {
    value += (popcnt(g_game.pieces[g_game.who2move][p]) -
              popcnt(g_game.pieces[!g_game.who2move][p])) *
             material[p];
  }
  return value;
}
