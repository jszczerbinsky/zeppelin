#include "main.h"

void perft(int depth, int *nodes, int *leafnodes) {
  if (depth == 0) {
    (*nodes)++;
    (*leafnodes)++;
    return;
  }

  MoveList movelist;
  BitBrd attacksbbrd;
  genmoves(g_game.who2move, &movelist, &attacksbbrd);

  if (movelist.cnt == 0) {
    (*nodes)++;
    (*leafnodes)++;
    return;
  }

  for (int i = 0; i < movelist.cnt; i++) {
    makemove(movelist.move[i]);

    if (lastmovelegal())
      perft(depth - 1, nodes, leafnodes);
    unmakemove();
  }
}
