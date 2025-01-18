#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#define TOK_DELIMS " \n"

#define OPT_CHECK 0

typedef struct {
  int opttype;
  const char *optname;
  void *valptr;
  const char *defstr;
} UciOpt;

const UciOpt opts[] = {
    {
        .opttype = OPT_CHECK,
        .optname = "Debug_DisableNMP",
        .valptr = &g_set.disbl_nmp,
        .defstr = "false",
    },
    {
        .opttype = OPT_CHECK,
        .optname = "Debug_DisableTT",
        .valptr = &g_set.disbl_tt,
        .defstr = "false",
    },
    {
        .opttype = OPT_CHECK,
        .optname = "Debug_DisablePVS",
        .valptr = &g_set.disbl_pvs,
        .defstr = "false",
    },
    {
        .opttype = OPT_CHECK,
        .optname = "Debug_DisableLMR",
        .valptr = &g_set.disbl_lmr,
        .defstr = "false",
    },
    {
        .opttype = OPT_CHECK,
        .optname = "Debug_DisableAspWnd",
        .valptr = &g_set.disbl_aspwnd,
        .defstr = "false",
    },

};

int g_ucidebug = 0;

static void respond2uci() {
  printf("id name testengine\n");
  printf("id author Jakub Szczerbinski\n");

  for (int i = 0; i < sizeof(opts) / sizeof(UciOpt); i++) {
    const UciOpt *opt = opts + i;

    const char *typename = "check";

    printf("option name %s type %s default %s\n", opt->optname, typename,
           opt->defstr);
  }

  printf("uciok\n");
  fflush(stdout);
}

static void respond2setoption(char *token) {
  if (!token || !equals(token, "name")) {
    return;
  }

  token = nexttok();

  for (int i = 0; i < sizeof(opts) / sizeof(UciOpt); i++) {
    const UciOpt *opt = opts + i;

    if (equals(token, opt->optname)) {

      token = nexttok();

      if (!token || !equals(token, "value")) {
        return;
      }

      token = nexttok();
      if (!token) {
        return;
      }

      switch (opt->opttype) {
      case OPT_CHECK:
        if (equals(token, "true")) {
          *((int *)opt->valptr) = 1;
        } else if (equals(token, "false")) {
          *((int *)opt->valptr) = 0;
        }
        break;
      default:
        break;
      }
      return;
    }
  }
}

static void respond2ucinewgame() { reset_hashtables(); }

static void respond2position(char *token) {
  if (equals(token, "startpos")) {
    token = nexttok();
    reset_game();
  } else if (equals(token, "fen")) {
    token = nexttok_untilend();
    token = parsefen(token);
  }

  if (token && equals(token, "moves")) {
    token = nexttok();
    while (token) {
      fflush(stdout);
      makemove(parsemove(token));
      token = nexttok();
    }
  }
}

static void respond2debug(char *token) {
  if (equals(token, "on"))
    g_ucidebug = 1;
  else if (equals(token, "off"))
    g_ucidebug = 0;
}

static void respond2isready() {
  printf("readyok\n");
  fflush(stdout);
}

static void respond2stop() { stop(STOP_MANUAL); }

static void runperft(char *token) {
  if (!token) {
    return;
  }

  int depth = atoi(token);

  MoveList movelist;
  BitBrd attacksbbrd;
  genmoves(g_game.who2move, &movelist, &attacksbbrd);

  int nodes = 0;

  for (int i = 0; i < movelist.cnt; i++) {
    int leafnodes = 0;
    makemove(movelist.move[i]);
    char buff[6];
    if (lastmovelegal()) {
      perft(depth - 1, &nodes, &leafnodes);
      move2str(buff, movelist.move[i]);
      printf("%s: %d\n", buff, leafnodes);
    }

    unmakemove();
  }
  printf("\nTotal: %d\n", nodes);
  fflush(stdout);
}

static void respond2go(char *token) {
  if (token && equals(token, "perft")) {
    return runperft(nexttok());
  }

  SearchSettings ss;
  ss.specificmoves.cnt = 0;

  int wtime = TIME_FOREVER;
  int btime = TIME_FOREVER;
  int winc = 0;
  int binc = 0;
  int movetime = TIME_FOREVER;

  int depth = DEPTH_INF;
  int nodes = 0;
  int ponder = 0;

  int readingmoves = 0;
  while (token) {
    if (equals(token, "wtime")) {
      readingmoves = 0;
      token = nexttok();
      if (token) {
        wtime = atoi(token);
      }
    } else if (equals(token, "btime")) {
      readingmoves = 0;
      token = nexttok();
      if (token) {
        btime = atoi(token);
      }
    } else if (equals(token, "winc")) {
      readingmoves = 0;
      token = nexttok();
      if (token) {
        winc = atoi(token);
      }
    } else if (equals(token, "binc")) {
      readingmoves = 0;
      token = nexttok();
      if (token) {
        binc = atoi(token);
      }
    } else if (equals(token, "moves2go")) {
      readingmoves = 0;
      token = nexttok();
      if (token) {
        // moves2go = atoi(token);
      }
    } else if (equals(token, "depth")) {
      readingmoves = 0;
      token = nexttok();
      if (token) {
        depth = atoi(token);
      }
    } else if (equals(token, "nodes")) {
      readingmoves = 0;
      token = nexttok();
      if (token) {
        nodes = atoi(token);
      }
    } else if (equals(token, "mate")) {
      readingmoves = 0;
      token = nexttok();
      if (token) {
        depth = atoi(token);
      }
    } else if (equals(token, "movetime")) {
      readingmoves = 0;
      token = nexttok();
      if (token) {
        movetime = atoi(token);
      }
    } else if (equals(token, "infinite")) {
      readingmoves = 0;
    } else if (equals(token, "searchmoves")) {
      readingmoves = 1;
    } else if (readingmoves) {
      pushmove(&ss.specificmoves, parsemove(token));
    }

    token = nexttok();
  }

  ss.depthlimit = depth;

  ss.nodeslimit = nodes;

  ss.timelimit = TIME_FOREVER;
  if (wtime != TIME_FOREVER && btime != TIME_FOREVER) {
    ss.timelimit = getsearchtime(wtime, btime, winc, binc);
  } else if (movetime) {
    ss.timelimit = movetime;
  }

  printf("info string timelimit %ld\n", ss.timelimit);

  search(&ss);
}

static int next_cmd(char *buff, int len) {
  char *token = strtok(buff, " \n");
  if (!token)
    return 0;

  if (equals(token, "uci"))
    respond2uci();
  else if (equals(token, "ucinewgame"))
    respond2ucinewgame();
  else if (equals(token, "go"))
    respond2go(nexttok());
  else if (equals(token, "stop"))
    respond2stop();
  else if (equals(token, "position"))
    respond2position(nexttok());
  else if (equals(token, "debug"))
    respond2debug(nexttok());
  else if (equals(token, "isready"))
    respond2isready();
  else if (equals(token, "setoption"))
    respond2setoption(nexttok());
  else if (equals(token, "quit"))
    return 1;

  return 0;
}

void uci_start() {
  g_mode = MODE_UCI;
  respond2uci();

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
