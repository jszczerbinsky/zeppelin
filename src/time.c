#include "main.h"

long getsearchtime(long wtime, long btime, long winc __attribute__((unused)),
                   long binc __attribute__((unused))) {
  long ptime;

  if (g_game.who2move == WHITE) {
    ptime = wtime;
    // pinc = winc;
  } else {
    ptime = btime;
    // pinc = binc;
  }

  int predictedlen;
  if (g_gamestate->fullmove <= 30) {
    predictedlen = 40;
  } else {
    predictedlen = g_gamestate->fullmove + 10;
  }

  return ptime / (predictedlen - g_gamestate->fullmove);
}
