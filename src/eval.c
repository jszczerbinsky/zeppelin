#include "main.h"

static const int doubledval = -15;

static const int moveval = 10;

static const int centerval = 10;
static const int excenterval = 3;

static const int sqrattackval = 2;

static const int kingshieldval = 10;

static const BitBrd pawnadvorder[2][6] = {
    {RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7},
    {RANK_7, RANK_6, RANK_5, RANK_4, RANK_3, RANK_2},
};
static const int pawnadvval[6] = {0, 0, 0, 20, 80, 150};

static const BitBrd files[8] = {FILE_A, FILE_B, FILE_C, FILE_D,
                                FILE_E, FILE_F, FILE_G, FILE_H};

static const int doublerookval = 80;

static const int rooklinewithkingval = 40;

static const int rooksemiopenline = 100;
static const int rookopenline = 200;

int evaluate_terminalpos(int pliescnt) {
  if (get_under_check_cnt() > 0) {
    return SCORE_CHECKMATED + pliescnt;
  } else {
    return 0;
  }
}

int evaluate(int pliescnt) {
  const int player = g_game.who2move;
  const int enemy = !player;

  /*MoveList pmoves;
  BitBrd pattacks;
  gen_moves(player, &pmoves, &pattacks, GEN_ALL, 0);

  int plegal = 0;
  for (int i = 0; i < pmoves.cnt; i++) {
    makemove(pmoves.move[i]);
    if (lastmovelegal())
      plegal++;
    unmakemove();
  }

  if (plegal == 0) {
    return evaluate_terminalpos(pliescnt);
  }

  MoveList emoves;
  BitBrd eattacks;
  gen_moves(enemy, &emoves, &eattacks, GEN_ALL, 0);*/

  int pkingsqr = bbrd2sqr(g_game.pieces[player][KING]);
  int ekingsqr = bbrd2sqr(g_game.pieces[enemy][KING]);

  int value = 0;

  /*value += (pmoves.cnt - emoves.cnt) * moveval;

  value += (popcnt(pattacks) - popcnt(eattacks)) * sqrattackval;*/

  int doubled_player = 0;
  int doubled_enemy = 0;
  for (int i = 0; i < 8; i++) {
    const BitBrd files[] = {FILE_A, FILE_B, FILE_C, FILE_D,
                            FILE_E, FILE_F, FILE_G, FILE_H};

    int player_inrow = popcnt(g_game.pieces[player][PAWN] & files[i]);
    int enemy_inrow = popcnt(g_game.pieces[enemy][PAWN] & files[i]);

    if (player_inrow > 1) {
      doubled_player += player_inrow;
    }
    if (enemy_inrow > 1) {
      doubled_enemy += enemy_inrow;
    }
  }

  value += (doubled_player - doubled_enemy) * doubledval;

  /*const BitBrd center = 0x1818000000ULL;
  const BitBrd excenter = 0x3c24243c0000ULL;
  value += (popcnt(pattacks & center) - popcnt(eattacks & center)) &
  centerval; value += (popcnt(pattacks & excenter) - popcnt(eattacks &
  excenter)) & excenterval;*/

  value += (popcnt(g_precomp.kingmask[pkingsqr] & g_game.piecesof[player]) -
            popcnt(g_precomp.kingmask[ekingsqr] & g_game.piecesof[enemy])) *
           kingshieldval;

  for (int i = 0; i < 6; i++) {
    value += pawnadvval[i] *
             popcnt(g_game.pieces[player][PAWN] & pawnadvorder[player][i]);
    value -= pawnadvval[i] *
             popcnt(g_game.pieces[enemy][PAWN] & pawnadvorder[enemy][i]);
  }

  for (int p = 0; p < PIECE_MAX; p++) {
    value +=
        (popcnt(g_game.pieces[player][p]) - popcnt(g_game.pieces[enemy][p])) *
        material[p];
  }

  for (int i = 0; i < 8; i++) {
    int prooks =
        popcnt((g_game.pieces[player][ROOK] | g_game.pieces[player][QUEEN]) &
               files[i]);
    int erooks = popcnt(
        (g_game.pieces[enemy][ROOK] | g_game.pieces[enemy][QUEEN]) & files[i]);

    if (prooks > 1) {
      value += prooks * doublerookval;
    }
    if (erooks > 1) {
      value -= erooks * doublerookval;
    }

    if (g_game.pieces[enemy][KING] & files[i]) {
      value += prooks * rooklinewithkingval;
    }
    if (g_game.pieces[player][KING] & files[i]) {
      value -= erooks * rooklinewithkingval;
    }

    int pawns = popcnt(g_game.pieces[ANY][PAWN] & files[i]);
    if (pawns == 0) {
      value += (prooks - erooks) * rookopenline;
    } else if (pawns == 1) {
      value += (prooks - erooks) * rooksemiopenline;
    }
  }

  return value;
}
