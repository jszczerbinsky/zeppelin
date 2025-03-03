#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

static void finishsending() {
  printf("END\n");
  fflush(stdout);
}

static void sendboard() {
  printf("{\n");

  printf("\"hash\": %lu,\n", g_gamestate->hash);

  printf("\"wpawn\": %lu,\n", g_game.pieces[WHITE][PAWN]);
  printf("\"wking\": %lu,\n", g_game.pieces[WHITE][KING]);
  printf("\"wknight\": %lu,\n", g_game.pieces[WHITE][KNIGHT]);
  printf("\"wbishop\": %lu,\n", g_game.pieces[WHITE][BISHOP]);
  printf("\"wrook\": %lu,\n", g_game.pieces[WHITE][ROOK]);
  printf("\"wqueen\": %lu,\n", g_game.pieces[WHITE][QUEEN]);
  printf("\"bpawn\": %lu,\n", g_game.pieces[BLACK][PAWN]);
  printf("\"bking\": %lu,\n", g_game.pieces[BLACK][KING]);
  printf("\"bknight\": %lu,\n", g_game.pieces[BLACK][KNIGHT]);
  printf("\"bbishop\": %lu,\n", g_game.pieces[BLACK][BISHOP]);
  printf("\"brook\": %lu,\n", g_game.pieces[BLACK][ROOK]);
  printf("\"bqueen\": %lu,\n", g_game.pieces[BLACK][QUEEN]);

  printf("\"player\": \"%s\",\n", g_game.who2move == WHITE ? "w" : "b");

  printf("\"wk\": %s,\n", CANCASTLE_WK(g_gamestate) ? "true" : "false");
  printf("\"wq\": %s,\n", CANCASTLE_WQ(g_gamestate) ? "true" : "false");
  printf("\"bk\": %s,\n", CANCASTLE_BK(g_gamestate) ? "true" : "false");
  printf("\"bq\": %s,\n", CANCASTLE_BQ(g_gamestate) ? "true" : "false");

  printf("\"ep\": %lu,\n", g_gamestate->epbbrd);

  printf("\"halfmove\": %u,\n", g_gamestate->halfmove);
  printf("\"fullmove\": %u\n", g_gamestate->fullmove);

  printf("}\n");
}

static void respond2getmoves() {
  MoveList movelist;
  BitBrd attacksbbrd;
  genmoves(g_game.who2move, &movelist, &attacksbbrd);

  printf("[\n");

  MoveList legallist;
  legallist.cnt = 0;

  for (int i = 0; i < movelist.cnt; i++) {
    makemove(movelist.move[i]);
    int is_legal = lastmovelegal();
    unmakemove();

    if (is_legal) {
      pushmove(&legallist, movelist.move[i]);
    }
  }

  for (int i = 0; i < legallist.cnt; i++) {
    char buff[8];
    move2str(buff, legallist.move[i]);

    printf("\"%s\"", buff);

    if (i != legallist.cnt - 1)
      printf(",\n");
    printf(" ");
  }

  printf("]\n");

  finishsending();
}

static void respond2getboard() {
  sendboard();
  finishsending();
}

static void respond2eval() {
  int score = evaluate(0);
  printf("{\"score\": %d}\n", score);
  finishsending();
}

static void respond2perft(char *depthstr) {
  int depth = atoi(depthstr);
  int nodes = 0;
  int leafnodes = 0;

  perft(depth, &nodes, &leafnodes);

  printf("{\"nodes\":%d}\n", nodes);
  finishsending();
}

static void respond2getscoreinfo(int score) {
  if (score == 0) {
    printf("{\"type\": \"draw\"}\n");
  } else if (score == SCORE_ILLEGAL || score == -SCORE_ILLEGAL) {
    printf("{\"type\": \"illegal\"}\n");
  } else if (score >= SCORE_CHECKMATE_BOUND && score <= SCORE_CHECKMATE) {
    printf("{\"type\": \"mate\"}\n");
  } else if (score <= -SCORE_CHECKMATE_BOUND && score >= -SCORE_CHECKMATE) {
    printf("{\"type\": \"mated\"}\n");
  } else if (score > 0) {
    printf("{\"type\": \"advantage\"}\n");
  } else {
    printf("{\"type\": \"disadvantage\"}\n");
  }
  finishsending();
}

static int next_cmd(char *buff, int len) {
  char *token = strtok(buff, " \n");
  if (!token)
    return 0;

  if (equals(token, "loadfen"))
    parsefen(nexttok_untilend());
  else if (equals(token, "makemove"))
    makemove(parsemove(nexttok()));
  else if (equals(token, "unmakemove"))
    unmakemove();
  else if (equals(token, "getboard"))
    respond2getboard();
  else if (equals(token, "getmoves"))
    respond2getmoves();
  else if (equals(token, "perft"))
    respond2perft(nexttok());
  else if (equals(token, "eval"))
    respond2eval();
  else if (equals(token, "getscoreinfo"))
    respond2getscoreinfo(atoi(nexttok()));
  else if (equals(token, "ttactive"))
    g_set.disbl_tt = !atoi(nexttok());
  else if (equals(token, "nmpactive"))
    g_set.disbl_nmp = !atoi(nexttok());
  else if (equals(token, "pvsactive"))
    g_set.disbl_pvs = !atoi(nexttok());
  else if (equals(token, "lmractive"))
    g_set.disbl_lmr = !atoi(nexttok());
  else if (equals(token, "aspwndactive"))
    g_set.disbl_aspwnd = !atoi(nexttok());

  else if (equals(token, "quit"))
    return 1;

  return 0;
}

static void printdbg(const char *format, ...) {
  printf("INFO");
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  putchar('\n');
}

void debug_start() {
  g_mode = MODE_DEBUG;
  g_printdbg = &printdbg;

  g_set.ttbytes = 20000000;
  ttinit();

  size_t buffsize = 256;
  char *buff = malloc(buffsize * sizeof(char));

  int quit = 0;
  int len = 0;

  while (!quit) {
    if ((len = getline(&buff, &buffsize, stdin)) == -1)
      quit = 1;
    else
      quit = next_cmd(buff, len);
  }

  free(buff);
}
