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

static void respond2go(char *token) {
  if (equals(token, "perft")) {
    token = nexttok();
    if (!token) {
      return;
    }

    int depth = atoi(token);

    MoveList movelist;
    BitBrd attacksbbrd;
    genmoves(g_game.who2move, &movelist, &attacksbbrd);

    int nodes = 0;

    for (int i = 0; i < movelist.cnt; i++) {
      Game backup;
      memcpy(&backup, &g_game, sizeof(Game));

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
  } else {
    while (token)
      token = nexttok();
    search(999);
  }
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
