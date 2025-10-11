#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#define TOK_DELIMS " \n"

#define OPT_CHECK 0
#define OPT_SPIN 1

typedef struct {
  int opttype;
  const char *optname;
  void *valptr;
  int valmul;
  const char *defstr;
  const char *minstr;
  const char *maxstr;
  void (*onchange)();
} UciOpt;

void onhashchange() {
  ttfree();
  ttinit();
}

const UciOpt opts[] = {
    {
        .opttype = OPT_CHECK,
        .optname = "Debug_DisableNMP",
        .valptr = &g_set.disbl_nmp,
        .defstr = "false",
        .onchange = NULL,
    },
    {
        .opttype = OPT_CHECK,
        .optname = "Debug_DisableTT",
        .valptr = &g_set.disbl_tt,
        .defstr = "false",
        .onchange = NULL,
    },
    {
        .opttype = OPT_CHECK,
        .optname = "Debug_DisablePVS",
        .valptr = &g_set.disbl_pvs,
        .defstr = "false",
        .onchange = NULL,
    },
    {
        .opttype = OPT_CHECK,
        .optname = "Debug_DisableLMR",
        .valptr = &g_set.disbl_lmr,
        .defstr = "false",
        .onchange = NULL,
    },
    {
        .opttype = OPT_CHECK,
        .optname = "Debug_DisableAspWnd",
        .valptr = &g_set.disbl_aspwnd,
        .defstr = "false",
        .onchange = NULL,
    },
    {
        .opttype = OPT_CHECK,
        .optname = "Debug_DisableAspWnd",
        .valptr = &g_set.disbl_aspwnd,
        .defstr = "false",
        .onchange = NULL,
    },
    {
        .opttype = OPT_CHECK,
        .optname = "Debug_DisableAspWnd",
        .valptr = &g_set.disbl_aspwnd,
        .defstr = "false",
        .onchange = NULL,
    },
    {
        .opttype = OPT_CHECK,
        .optname = "UCI_ShowCurrLine",
        .valptr = &g_set.print_currline,
        .defstr = "false",
        .onchange = NULL,
    },
    {
        .opttype = OPT_SPIN,
        .optname = "Hash",
        .valptr = &g_set.ttbytes,
        .valmul = 1000000,
        .defstr = "20",
        .minstr = "1",
        .maxstr = "1000",
        .onchange = onhashchange,
    },

};

int g_ucidebug = 0;

#ifdef DEBUG_INTERFACE
static void printdbg(const char *format, ...) {
  printf("info string ");
  va_list args;
  va_start(args, format);
  vprintf(format, args);
  va_end(args);
  putchar('\n');
}
#endif

static void printline(const MoveList *line) {
  for (int i = 0; i < line->cnt; i++) {
    char buff[6];
    move2str(buff, line->move[i]);
    printf(" %s", buff);
  }
}

static void printscore(int score) {
  printf("score ");
  if (IS_CHECKMATE(score) || IS_CHECKMATE(-score)) {
    int mul = 1;

    if (score < 0) {
      score *= -1;
      mul = -1;
    }

    int moves = (1 + SCORE_CHECKMATE - score) / 2;
    printf("mate %d", moves * mul);
  } else {
    printf("cp %d", score);
  }
}

static void on_iterfinish(const SearchInfo *si, size_t ttused, size_t ttsize,
                          int score) {
  size_t hashfull = (ttused * 1000UL) / ttsize;

  printf("info depth %d seldepth %d nps %d tbhits %d hashfull %zu nodes %ld ",
         si->iter_depth, si->iter_highest_depth, calcnps(), si->iter_tbhits,
         hashfull, si->iter_visited_nodes);
  printscore(score);
  printf(" pv");
  printline(&si->prev_iter_pv);
  putchar('\n');
  fflush(stdout);
}

static void on_move(const SearchInfo *si, size_t ttused, size_t ttsize,
                    int score) {
  long hashfull = ((long)((long)ttused * 1000L)) / (long)ttsize;

  printf("info nodes %ld currmove %s currmovenumber %d nps %d hashfull %ld ",
         si->iter_visited_nodes, si->rootmove_str, si->rootmove_n, calcnps(),
         hashfull);

  if (si->currline.cnt > 0) {
    printf("currline");
    printline(&si->currline);
    printf(" ");
  }
  printscore(score);
  putchar('\n');
  fflush(stdout);
}

static void on_finish(const SearchInfo *si) {
  char buff[6];
  move2str(buff, si->prev_iter_pv.move[0]);
  printf("bestmove %s\n", buff);
  fflush(stdout);
}

static void respond2uci() {
  printf("id name Zeppelin " PROGRAM_VERSION "\n");
  printf("id author Jakub Szczerbinski\n");

  for (size_t i = 0; i < sizeof(opts) / sizeof(UciOpt); i++) {
    const UciOpt *opt = opts + i;

    switch (opt->opttype) {
    case OPT_SPIN:
      printf("option name %s type spin default %s min %s max %s\n",
             opt->optname, opt->defstr, opt->minstr, opt->maxstr);
      break;
    case OPT_CHECK:
      printf("option name %s type check default %s\n", opt->optname,
             opt->defstr);
      break;
    }
  }

  printf("uciok\n");
  fflush(stdout);
}

static void respond2setoption(char *token) {
  if (!token || !equals(token, "name")) {
    return;
  }

  token = nexttok();

  for (size_t i = 0; i < sizeof(opts) / sizeof(UciOpt); i++) {
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
      case OPT_SPIN:
        *((long *)opt->valptr) = atoi(token) * opt->valmul;
        break;
      default:
        break;
      }

      if (opt->onchange) {
        opt->onchange();
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

static void respond2stop() { stop(); }

static void runperft(char *token) {
  if (!token) {
    return;
  }

  int depth = atoi(token);

  MoveList movelist;
  BitBrd attacksbbrd;
  gen_moves(g_game.who2move, &movelist, &attacksbbrd, GEN_ALL, 0);

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
    runperft(nexttok());
    return;
  }

  SearchSettings ss;
  ss.startdepth = 1;
  ss.on_finish = &on_finish;
  ss.on_rootmove = g_set.print_currline ? &on_move : NULL;
  ss.on_nonrootmove = g_set.print_currline ? &on_move : NULL;
  ss.on_iterfinish = &on_iterfinish;
  ss.specificmoves.cnt = 0;

  int wtime = TIME_FOREVER;
  int btime = TIME_FOREVER;
  int winc = 0;
  int binc = 0;
  int movetime = TIME_FOREVER;

  int depth = DEPTH_INF;
  int nodes = 0;
  // int ponder = 0;

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

  PRINTDBG("timelimit %ld", ss.timelimit);

  search(&ss);
}

static int next_cmd(char *buff) {
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
#ifdef DEBUG_INTERFACE
  g_printdbg = &printdbg;
#endif

  g_set.ttbytes = 20000000;
  ttinit();

  g_mode = MODE_UCI;
  respond2uci();

  const size_t buffsize = 4096;
  char buff[buffsize];

  int quit = 0;

  while (!quit) {
    if (fgets(buff, (int)buffsize, stdin) == NULL)
      quit = 1;
    else {
      // todo check if all string fit in the buff
      quit = next_cmd(buff);
    }
  }

  ttfree();
}
