#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "main.h"

#define ON_FINISH() (ss.on_finish ? (*ss.on_finish)(&si) : "")
#define ON_ROOTMOVE(score)                                                     \
  (ss.on_rootmove ? (*ss.on_rootmove)(&si, ttused, ttsize, score) : "")
#define ON_NONROOTMOVE(score)                                                  \
  (ss.on_nonrootmove ? (*ss.on_nonrootmove)(&si, ttused, ttsize, score) : "")
#define ON_ITERFINISH(score)                                                   \
  (ss.on_iterfinish ? (*ss.on_iterfinish)(&si, ttused, ttsize, score) : "")

#define MIN_PRIORITY (-99999)

#define NODE_INSIDEWND 0
#define NODE_FAILH 1
#define NODE_FAILL 2
#define NODE_GAMEFINISHED 3

#define TT_EXACT 0
#define TT_LOWERBOUND 1
#define TT_UPPERBOUND 2

typedef struct {
  int used;
  BitBrd hash;
  int type;
  int value;
  int depth;
  Move bestmove;
} TT;

static TT *tt = NULL;
static int ttsize;
static int ttused = 0;

static SearchInfo si;
static SearchSettings ss;
static pthread_t search_thread;
static pthread_t supervisor_thread;

static int manualstop = 0;

void ttinit() {
  ttsize = g_set.ttbytes / sizeof(TT);
  ttused = 0;
  tt = calloc(ttsize, sizeof(TT));
}

void ttfree() { free(tt); }

static const TT *ttread(BitBrd hash, int depth) {
  TT *ttentry = tt + (hash % ttsize);
  if (ttentry->used && ttentry->hash == hash && ttentry->depth >= depth) {
    return ttentry;
  }
  return NULL;
}

static void ttwrite(BitBrd hash, int type, int depth, int value,
                    Move bestmove) {
  TT *ttentry = tt + (hash % ttsize);

  if (!ttentry->used) {
    ttused++;
  }

  ttentry->used = 1;
  ttentry->hash = hash;
  ttentry->type = type;
  ttentry->depth = depth;
  ttentry->value = value;
  ttentry->bestmove = bestmove;
}

int max(int a, int b) {
  if (a > b)
    return a;
  return b;
}

int min(int a, int b) {
  if (a < b)
    return a;
  return b;
}

int calcnps() {
  clock_t now = clock();

  double seconds = (now - si.nps_lastcalc) / (double)CLOCKS_PER_SEC;
  int nodesdiff = si.iter_visited_nodes - si.nps_lastnodes;

  int nps = nodesdiff / seconds;

  si.nps_lastnodes = si.iter_visited_nodes;
  si.nps_lastcalc = clock();

  return nps;
}

static void addkiller(int depth, Move move) {
  for (int i = 0; i < KILLER_MAX; i++) {
    if (si.iter_killers[depth][i] == move)
      return;
    if (si.iter_killers[depth][i] == NULLMOVE) {
      si.iter_killers[depth][i] = move;
      return;
    }
  }
}

static int iskiller(int depth, Move move) {
  for (int i = 0; i < KILLER_MAX; i++)
    if (si.iter_killers[depth][i] == move)
      return 1;
  return 0;
}

void reset_hashtables() {}

static int get_priority(Move move, Move ttbest) {

  const int pvpriority = 1000000;
  const int ttpriority = 100000;
  const int captpriority = 10000;
  const int killerpriority = 1;
  const int normalpriority = 0;

  if (move == ttbest) {
    return ttpriority;
  }

  int ply = si.currline.cnt;
  if (ply < si.prev_iter_pv.cnt && move == si.prev_iter_pv.move[ply]) {
    return pvpriority;
  }

  if (iskiller(ply, move)) {
    return killerpriority;
  }

  int isnormal = 1;

  int diff = 0;
  if (IS_CAPT(move)) {
    diff = material[GET_CAPT_PIECE(move)] - material[GET_MOV_PIECE(move)];
    isnormal = 0;
  }

  if (IS_PROM(move)) {
    diff += material[GET_PROM_PIECE(move)];
    isnormal = 0;
  }

  if (isnormal) {
    return normalpriority;
  }

  return captpriority + diff;
}

static void order(MoveList *movelist, int curr, Move ttbest) {
  int best_i = -1;
  int best_priority = MIN_PRIORITY;

  for (int i = curr; i < movelist->cnt; i++) {
    int priority = get_priority(movelist->move[i], ttbest);
    if (priority > best_priority) {
      best_i = i;
      best_priority = priority;
    }
  }
  Move best = movelist->move[best_i];
  movelist->move[best_i] = movelist->move[curr];
  movelist->move[curr] = best;
}

typedef struct {
  MoveList availmoves;
  Move bestmove;
  int nodetype;
  int legalcnt;
  int score;
} NodeInfo;

int quiescence(int alpha, int beta, int depthleft) {
  float standpat = evaluate(si.currline.cnt);
  int best = standpat;
  if (standpat >= beta || depthleft == 0) {
    return beta;
  }

  si.iter_visited_nodes++;
  alpha = max(standpat, alpha);

  MoveList availmoves;
  BitBrd attackbbrd;
  gencapt(g_game.who2move, &availmoves, &attackbbrd);

  for (int i = 0; i < availmoves.cnt; i++) {
    if (lastmovelegal()) {
      order(&availmoves, i, NULLMOVE);
      Move currmove = availmoves.move[i];

      pushmove(&si.currline, currmove);
      makemove(currmove);
      int score = -quiescence(-beta, -alpha, depthleft - 1);
      unmakemove();
      popmove(&si.currline);

      if (score >= beta) {
        return beta;
      }
      best = max(score, best);
      alpha = max(best, alpha);
    }
  }
  return best;
}

int negamax(int alpha, int beta, int depthleft);

void analyze_node(NodeInfo *ni, int depthleft, int *alpha, int beta,
                  Move ttbest) {
  ni->legalcnt = 0;
  ni->nodetype = NODE_FAILL;
  ni->bestmove = NULLMOVE;
  int score = SCORE_ILLEGAL;

  if (!g_set.disbl_nmp && !undercheck() && si.currline.cnt > 3 &&
      si.currline.move[si.currline.cnt - 1] != NULLMOVE &&
      evaluate(si.currline.cnt) >= beta) {
    makemove(NULLMOVE);
    pushmove(&si.currline, NULLMOVE);
    int nmpscore = -negamax(-beta, -beta + 1, depthleft - 1 - 3);
    popmove(&si.currline);
    unmakemove();

    if (nmpscore >= beta) {
      ni->nodetype = 17;
      ni->score = beta;
      return;
    }
  }

  BitBrd attackbbrd;
  genmoves(g_game.who2move, &ni->availmoves, &attackbbrd);

  for (int i = 0; i < ni->availmoves.cnt; i++) {
    order(&ni->availmoves, i, ttbest);
    Move currmove = ni->availmoves.move[i];

    if (si.currline.cnt == 0 && ss.specificmoves.cnt > 0 &&
        !containsmove(&ss.specificmoves, currmove)) {
      continue;
    }

    pushmove(&si.currline, currmove);
    makemove(currmove);

    if (lastmovelegal()) {
      ni->legalcnt++;

      int ext = 0;
      if (si.currext < 3 && depthleft == 1 && undercheck())
        ext++;
      int red = 0;
      if (!g_set.disbl_lmr && si.currline.cnt > 3 && !IS_CAPT(currmove) &&
          !undercheck() && ni->legalcnt > 6) {
        red++;
      }

      si.currext += ext;
      int movescore;
      int pvsallowed = i > 0 && !g_set.disbl_pvs && !g_gamestate->isendgame;
      if (pvsallowed) {
        movescore = -negamax(-*alpha - 1, -*alpha, depthleft + ext - red - 1);
      }
      if (!pvsallowed || (movescore > *alpha && movescore < beta)) {
        movescore = -negamax(-beta, -*alpha, depthleft + ext - red - 1);
      }
      si.currext -= ext;

      if (si.currline.cnt == 1) {
        move2str(si.rootmove_str, ni->availmoves.move[i]);
        si.rootmove_n++;
        ON_ROOTMOVE(movescore);
      } else {
        ON_NONROOTMOVE(movescore);
      }

      score = max(score, movescore);

      if (score >= beta) {
        unmakemove();
        popmove(&si.currline);

        if (IS_SILENT(currmove)) {
          addkiller(si.currline.cnt, currmove);
        }

        ni->nodetype = NODE_FAILH;
        // ni->score = beta;
        ni->score = score;
        return;
      }

      if (score > *alpha) {
        ni->nodetype = NODE_INSIDEWND;
        ni->bestmove = currmove;
        *alpha = score;
      }
    }

    unmakemove();
    popmove(&si.currline);
  }

  if (ni->legalcnt == 0) {
    ni->nodetype = NODE_GAMEFINISHED;
    ni->score = evaluate(si.currline.cnt);
  } else {
    ni->score = score;
  }
}

int negamax(int alpha, int beta, int depthleft) {
  const BitBrd hash = g_gamestate->hash;

  int rep = getrepetitions();
  if (rep >= 3) {
    return 0;
  }

  Move ttbest = NULLMOVE;
  if (!g_set.disbl_tt && si.currline.cnt > 0) {
    const TT *ttentry = ttread(hash, depthleft);
    if (ttentry) {
      ttbest = ttentry->bestmove;
      si.iter_tbhits++;

      switch (ttentry->type) {
      case TT_EXACT:
        return ttentry->value;
        break;
      case TT_UPPERBOUND:
        beta = min(beta, ttentry->value);
        break;
      case TT_LOWERBOUND:
        alpha = max(alpha, ttentry->value);
        break;
      }
      if (alpha >= beta) {
        return ttentry->value;
      }
    }
  }

  if (si.currline.cnt > si.iter_highest_depth) {
    si.iter_highest_depth = si.currline.cnt;
  }

  if (depthleft <= 0) {
    return quiescence(alpha, beta, 4);
  }

  si.iter_visited_nodes++;

  NodeInfo ni;
  analyze_node(&ni, depthleft, &alpha, beta, ttbest);

  if (si.currline.cnt == 0) {
    si.iter_bestmove = ni.bestmove;
    si.root_nodetype = ni.nodetype;
  }

  switch (ni.nodetype) {
  case NODE_FAILL:
    ttwrite(hash, TT_UPPERBOUND, depthleft, alpha, ni.bestmove);
    break;
  case NODE_FAILH:
    ttwrite(hash, TT_LOWERBOUND, depthleft, beta, ni.bestmove);
    break;
  case NODE_INSIDEWND:
  case NODE_GAMEFINISHED:           // Illegal (mated or stalemated)
    if (ss.specificmoves.cnt > 0) { // possible better move wasnt checked
      ttwrite(hash, TT_LOWERBOUND, depthleft, ni.score, ni.bestmove);
    } else {
      ttwrite(hash, TT_EXACT, depthleft, ni.score, ni.bestmove);
    }
    break;
  default:
    break;
  }

  return ni.score;
}

static void search_finish() {
  if (si.prev_iter_pv.cnt > 0) {
    ON_FINISH();
  } else {
    PRINTDBG("no legal move detected");
    fflush(stdout);
  }
  si.finished = 1;
}

static void recoverpv(MoveList *pv, int depth) {
  const TT *tt = ttread(g_gamestate->hash, depth);

  if (tt && tt->bestmove != NULLMOVE) {
    pushmove(pv, tt->bestmove);
    makemove(tt->bestmove);

    if (getrepetitions() < 3) {
      recoverpv(pv, depth - 1);
    }
    unmakemove();
  }
}

static void *search_subthread(void *arg) {
  int oldtype;
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

  int firsttime = 1;
  int lastscore;

  for (int depth = 1; depth <= ss.depthlimit; depth++) {
    si.iter_highest_depth = 0;
    si.currline.cnt = 0;
    si.iter_tbhits = 0;
    si.iter_visited_nodes = 0;
    si.nps_lastnodes = 0;
    si.nps_lastcalc = clock();
    si.rootmove_n = 0;
    si.currext = 0;
    si.iter_depth = depth;
    memset(&si.iter_killers, 0, sizeof(Move) * KILLER_MAX * MAX_PLY_PER_GAME);

    int score;
    if (g_set.disbl_aspwnd || firsttime || IS_CHECKMATE(lastscore)) {
      score = negamax(SCORE_ILLEGAL, -SCORE_ILLEGAL, depth);
      firsttime = 0;
    } else {
      int inbounds = 0;
      const int sizes[] = {25, 50, 100, 200, 400};
      const int sizes_max = sizeof(sizes) / sizeof(int);

      int asize_i = 0;
      int bsize_i = 0;
      do {
        int alpha;
        int beta;

        if (asize_i < sizes_max) {
          alpha = lastscore - sizes[asize_i];
        } else {
          alpha = SCORE_ILLEGAL;
        }
        if (bsize_i < sizes_max) {
          beta = lastscore + sizes[bsize_i];
        } else {
          beta = -SCORE_ILLEGAL;
        }

        score = negamax(alpha, beta, depth);

        if (si.root_nodetype == NODE_FAILL) {
          asize_i++;
        } else if (si.root_nodetype == NODE_FAILH) {
          bsize_i++;
        } else {
          inbounds = 1;
        }
      } while (!inbounds);
    }
    lastscore = score;

    MoveList pv = {0};
    recoverpv(&pv, depth);
    if (pv.cnt == 0) {
      PRINTDBG("couldnt recover from pv!");
      fflush(stdout);
      pv.cnt = 1;
      pv.move[0] = si.iter_bestmove;
    }

    memcpy(&si.prev_iter_pv, &pv, sizeof(MoveList));
    ON_ITERFINISH(score);

    if (IS_CHECKMATE(score)) {
      break;
    }
  }

  si.finished = 1;
  return 0;
}

static void *supervisor_subthread(void *arg) {
  int oldtype;
  pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

  clock_t start_time = clock() / CLOCKS_PER_MS;

  pthread_create(&search_thread, NULL, search_subthread, NULL);

  while (1) {
    if (si.finished) {
      break;
    }
    if (ss.timelimit != TIME_FOREVER &&
        difftime(clock() / CLOCKS_PER_MS, start_time) >= ss.timelimit) {
      PRINTDBG("cancelling on time");
      break;
    }
    if (ss.nodeslimit != 0 && si.iter_visited_nodes >= ss.nodeslimit) {
      PRINTDBG("canceling on nodes limit");
      break;
    }
    if (manualstop) {
      PRINTDBG("canceling on time");
      break;
    }

    usleep(10);
  }
  fflush(stdout);

  while (si.prev_iter_pv.cnt == 0) {
    usleep(10);
  }

  pthread_cancel(search_thread);
  pthread_join(search_thread, NULL);

  search_finish();
  return 0;
}

void search(const SearchSettings *settings) {
  memset(&si, 0, sizeof(SearchInfo));

  memcpy(&ss, settings, sizeof(SearchSettings));

  manualstop = 0;
  si.finished = 0;
  pthread_create(&supervisor_thread, NULL, supervisor_subthread, NULL);
}

void stop(int origin) {
  PRINTDBG("canceling manually");
  fflush(stdout);
  manualstop = 1;
  pthread_join(supervisor_thread, NULL);
}
