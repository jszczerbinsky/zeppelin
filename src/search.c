#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "main.h"

#define MIN_PRIORITY (-99999)

#define KILLER_MAX 5

#define NODE_INSIDEWND 0
#define NODE_FAILH 1
#define NODE_FAILL 2
#define NODE_ILLEGAL 3

typedef struct {
  int movenum;
  char movestr[6];
  int nodes;
  MoveList currline;
  MoveList prev_pv;
  Move bestmove;
  int requesteddepth;
  int tbhits;
  int maxdepth;
  Move killers[MAX_PLY_PER_GAME][KILLER_MAX];
  int exttotal;
} SearchInfo;

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

#define TT_SIZE 2000000

static TT tt[TT_SIZE];
static int ttused = 0;

static SearchInfo si;
static pthread_t search_thread;
static pthread_t time_thread;

static int stopping = 0;

static const TT *ttread(BitBrd hash, int depth) {
  TT *ttentry = tt + (hash % TT_SIZE);
  if (ttentry->used && ttentry->hash == hash && ttentry->depth >= depth) {
    return ttentry;
  }
  return NULL;
}

static void ttwrite(BitBrd hash, int type, int depth, int value,
                    Move bestmove) {
  TT *ttentry = tt + (hash % TT_SIZE);

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

static void printinfo_regular(int score) {
  int hashfull = (ttused * 1000) / TT_SIZE;

  printf("info nodes %d currmove %s currmovenumber %d hashfull %d ", si.nodes,
         si.movestr, si.movenum, hashfull);

  if (si.currline.cnt > 0) {
    printf("currline");
    printline(&si.currline);
    printf(" ");
  }
  printscore(score);
  putchar('\n');
  fflush(stdout);
}

static void printinfo_final(int depth, int score) {
  int hashfull = (ttused * 1000) / TT_SIZE;

  printf("info depth %d seldepth %d tbhits %d hashfull %d nodes %d ", depth,
         si.maxdepth, si.tbhits, hashfull, si.nodes);
  printscore(score);
  printf(" pv");
  printline(&si.prev_pv);
  putchar('\n');
  fflush(stdout);
}

static void addkiller(int depth, Move move) {
  for (int i = 0; i < KILLER_MAX; i++) {
    if (si.killers[depth][i] == move)
      return;
    if (si.killers[depth][i] == NULLMOVE) {
      si.killers[depth][i] = move;
      return;
    }
  }
}

static int iskiller(int depth, Move move) {
  for (int i = 0; i < KILLER_MAX; i++)
    if (si.killers[depth][i] == move)
      return 1;
  return 0;
}

void reset_hashtables() {}

static int get_priority(Move move, Move ttbest) {

  const int pvpriority = 1000000;
  const int ttpriority = 100000;
  const int goodcaptpriority = 10000;
  const int equalcaptpriority = 2;
  const int killerpriority = 1;
  const int normalpriority = 0;
  const int badcaptpriority = -10000;

  if (move == ttbest) {
    return ttpriority;
  }

  int ply = si.currline.cnt;
  if (ply < si.prev_pv.cnt && move == si.prev_pv.move[ply]) {
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

  if (diff > 0) {
    return goodcaptpriority + diff;
  } else if (diff == 0) {
    return equalcaptpriority;
  } else {
    return badcaptpriority + diff;
  }
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

    Move best = movelist->move[best_i];
    movelist->move[best_i] = movelist->move[curr];
    movelist->move[curr] = best;
  }
}

typedef struct {
  MoveList availmoves;
  Move bestmove;
  int nodetype;
  int legalcnt;
  int score;
} NodeInfo;

int negamax(int alpha, int beta, int depthleft);

void analyze_node(NodeInfo *ni, int depthleft, int alpha, int beta,
                  Move ttbest) {
  ni->legalcnt = 0;
  ni->nodetype = NODE_FAILL;
  ni->bestmove = NULLMOVE;
  int score = SCORE_ILLEGAL;

  if (!g_set.disbl_nmp && !undercheck() &&
      (si.currline.cnt == 0 ||
       si.currline.move[si.currline.cnt - 1] != NULLMOVE)) {
    makemove(NULLMOVE);
    pushmove(&si.currline, NULLMOVE);
    int nmpscore = -negamax(-beta, -beta + 1, depthleft - 1 - 3);
    popmove(&si.currline);
    unmakemove();

    if (nmpscore >= beta) {
      ni->nodetype = NODE_FAILH;
      ni->score = beta;
      return;
    }
  }

  genmoves(g_game.who2move, &ni->availmoves);

  for (int i = 0; i < ni->availmoves.cnt; i++) {
    order(&ni->availmoves, i, ttbest);
    Move currmove = ni->availmoves.move[i];

    pushmove(&si.currline, currmove);
    makemove(currmove);

    if (lastmovelegal()) {
      ni->legalcnt++;

      int ext = 0;
      if (si.exttotal < 3 && depthleft == 1 && undercheck())
        ext++;

      si.exttotal += ext;
      int movescore = -negamax(-beta, -alpha, depthleft + ext - 1);
      si.exttotal -= ext;

      if (si.currline.cnt == 1) {
        move2str(si.movestr, ni->availmoves.move[i]);
        si.movenum++;
        printinfo_regular(movescore);
      } else {
        // printinfo_regular(movescore);
      }

      score = max(score, movescore);

      if (score >= beta) {
        unmakemove();
        popmove(&si.currline);

        if (IS_SILENT(currmove)) {
          addkiller(si.currline.cnt, currmove);
        }

        ni->nodetype = NODE_FAILH;
        ni->score = beta;
        return;
      }

      if (score > alpha) {
        ni->nodetype = NODE_INSIDEWND;
        ni->bestmove = currmove;
        alpha = score;
      }
    }

    unmakemove();
    popmove(&si.currline);
  }

  if (ni->legalcnt == 0) {
    ni->nodetype = NODE_ILLEGAL;
    ni->score = evaluate(si.currline.cnt);
  } else {
    ni->score = alpha;
  }
}

int negamax(int alpha, int beta, int depthleft) {
  const BitBrd hash = gethash();

  int rep = getrepetitions();
  if (rep >= 5) {
    // fivefold repetition - automatic draw
    return 0;
  } else if (rep >= 3) {
    // threefold repetition - player has to claim the draw if he wants to
    alpha = 0;
  }

  Move ttbest = NULLMOVE;
  if (!g_set.disbl_tt) {
    const TT *ttentry = ttread(hash, depthleft);
    if (ttentry) {
      ttbest = ttentry->bestmove;
      si.tbhits++;

      switch (ttentry->type) {
      case TT_EXACT:
        return ttentry->value;
        break;
      case TT_UPPERBOUND:
        alpha = max(alpha, ttentry->value);
        break;
      case TT_LOWERBOUND:
        beta = min(beta, ttentry->value);
        break;
      }
      if (alpha >= beta) {
        return ttentry->value;
      }
    }
  }

  if (si.currline.cnt > si.maxdepth) {
    si.maxdepth = si.currline.cnt;
  }

  if (depthleft <= 0) {
    return evaluate(si.currline.cnt);
  }

  si.nodes++;

  NodeInfo ni;
  analyze_node(&ni, depthleft, alpha, beta, ttbest);

  // Remember: old values of alpha and beta - don't use them after analyze_node

  if (si.currline.cnt == 0) {
    si.bestmove = ni.bestmove;
  }

  switch (ni.nodetype) {
  case NODE_FAILH:
    ttwrite(hash, TT_UPPERBOUND, depthleft, ni.score, ni.bestmove);
    break;
  case NODE_FAILL:
    ttwrite(hash, TT_LOWERBOUND, depthleft, ni.score, ni.bestmove);
    break;
  case NODE_INSIDEWND:
    ttwrite(hash, TT_EXACT, depthleft, ni.score, ni.bestmove);
    break;
  default: // Illegal (mated or stalemated)
    ttwrite(hash, TT_EXACT, depthleft, ni.score, ni.bestmove);
    break;
  }

  return ni.score;
}

static void search_finish() {
  if (si.bestmove != NULLMOVE) {
    char buff[6];
    move2str(buff, si.bestmove);
    // todo lock for printing
    printf("\nbestmove %s\n", buff);
    fflush(stdout);
  } else {
    printf("info string no legal move detected\n");
    fflush(stdout);
  }
}

static void recoverpv(MoveList *pv, int depth) {
  const TT *tt = ttread(gethash(), depth);

  if (tt) {
    pushmove(pv, tt->bestmove);
    makemove(tt->bestmove);

    if (getrepetitions() < 3) {
      recoverpv(pv, depth - 1);
    }
    unmakemove();
  }
}

static void *search_subthread(void *arg) {
  memset(tt, 0, sizeof(TT) * TT_SIZE);
  for (int depth = 1; depth <= si.requesteddepth; depth++) {
    si.maxdepth = 0;
    si.currline.cnt = 0;
    si.tbhits = 0;
    si.nodes = 0;
    si.movenum = 0;
    si.exttotal = 0;
    memset(&si.killers, 0, sizeof(Move) * KILLER_MAX * MAX_PLY_PER_GAME);

    int score = negamax(SCORE_ILLEGAL, -SCORE_ILLEGAL, depth);

    MoveList pv = {0};
    recoverpv(&pv, depth);

    memcpy(&si.prev_pv, &pv, sizeof(MoveList));
    printinfo_final(depth, score);
  }

  search_finish();
  return 0;
}

static void *time_subthread(void *arg) {
  time_t start_time = time(NULL);
  while (1) {
    if (difftime(time(NULL), start_time) >= 99999) {
      stop(STOP_TIME);
      break;
    }
    usleep(10);
  }
  return 0;
}

void search(int requesteddepth) {
  memset(&si, 0, sizeof(SearchInfo));

  si.requesteddepth = requesteddepth;

  pthread_create(&time_thread, NULL, time_subthread, NULL);
  pthread_create(&search_thread, NULL, search_subthread, NULL);
}

void stop(int origin) {
  if (!stopping) {
    stopping = 1;
    if (origin != STOP_TIME) {
      printf("info string canceling manually\n");
      fflush(stdout);
      pthread_cancel(time_thread);
      pthread_join(time_thread, NULL);
    } else {
      printf("info string canceling on time\n");
      fflush(stdout);
    }

    pthread_cancel(search_thread);
    pthread_join(search_thread, NULL);

    search_finish();
    stopping = 0;
  }
}
