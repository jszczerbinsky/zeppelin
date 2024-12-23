#include <stdio.h>

#include "main.h"

static const int piece_values[] = {100, 0, 300, 300, 500, 800};

int evaluate(int pliescnt) {
  const int player = g_game.who2move;
  const int enemy = !player;

  MoveList list;
  genmoves(player, &list);

  int legal = 0;

  for (int i = 0; i < list.cnt; i++) {
    makemove(list.move[i]);

    if (lastmovelegal())
      legal++;

    unmakemove();
  }

  if (legal == 0) {
    // char *names[] = {"white", "black"};
    // printf("info string %s can be checkmated in %d moves\n",
    //        names[g_game.who2move], pliescnt);

    if (sqr_attackedby(!g_game.who2move,
                       bbrd2sqr(g_game.pieces[g_game.who2move][KING]))) {

      return SCORE_CHECKMATED + pliescnt;
    } else {
      return 0;
    }
  }

  int material_diff = 0;

  for (int p = 0; p < PIECE_MAX; p++)
    material_diff +=
        (popcnt(g_game.pieces[player][p]) - popcnt(g_game.pieces[enemy][p])) *
        piece_values[p];

  return material_diff;
}
