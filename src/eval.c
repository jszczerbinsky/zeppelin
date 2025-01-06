#include "main.h"

static const int doubledval = -15;

static const int moveval = 10;

static const int centerval = 10;
static const int excenterval = 3;

int evaluate(int pliescnt) {
  const int player = g_game.who2move;
  const int enemy = !player;

  MoveList pmoves;
  genmoves(player, &pmoves);

  int plegal = 0;
  for (int i = 0; i < pmoves.cnt; i++) {
    makemove(pmoves.move[i]);
    if (lastmovelegal())
      plegal++;
    unmakemove();
  }

  if (plegal == 0) {
    if (undercheck()) {
      return SCORE_CHECKMATED + pliescnt;
    } else {
      return 0;
    }
  }

  MoveList emoves;
  genmoves(enemy, &emoves);

  int value = (pmoves.cnt - emoves.cnt) * moveval;

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

  const int centersqrs[] = {27, 28, 35, 36};
  for (int i = 0; i < 4; i++) {
    if (sqr_attackedby(player, centersqrs[i])) {
      value += centerval;
    }
    if (sqr_attackedby(enemy, centersqrs[i])) {
      value -= centerval;
    }
  }
  const int excentersqrs[] = {18, 19, 20, 21, 26, 29, 34, 37, 42, 43, 44, 45};
  for (int i = 0; i < 12; i++) {
    if (sqr_attackedby(player, excentersqrs[i])) {
      value += excenterval;
    }
    if (sqr_attackedby(enemy, excentersqrs[i])) {
      value -= excenterval;
    }
  }

  for (int p = 0; p < PIECE_MAX; p++)
    value +=
        (popcnt(g_game.pieces[player][p]) - popcnt(g_game.pieces[enemy][p])) *
        material[p];

  return value;
}
