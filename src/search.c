#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "main.h"

#define MIN_PRIORITY (-99999)

#define KILLER_MAX 5

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
} SearchInfo;

#define TT_EXACT 0
#define TT_ALPHA 1
#define TT_BETA 2

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

int negamax(int alpha, int beta, int depthleft, MoveList *pv) {
  const BitBrd hash = gethash();

  Move ttbest = NULLMOVE;
  const TT *ttentry = ttread(hash, depthleft);
  if (ttentry) {
    if (ttentry->type == TT_EXACT ||
        (ttentry->type == TT_ALPHA && ttentry->value <= alpha) ||
        (ttentry->type == TT_BETA && ttentry->value >= beta)) {
      pv->cnt = 1;
      pv->move[0] = ttentry->bestmove;
      si.tbhits++;
      return ttentry->value;
    }
    ttbest = ttentry->bestmove;
  }

  int score = SCORE_ILLEGAL;
  Move bestmove = NULLMOVE;

  int betacutoff = 0;
  int alpharisen = 0;

  if (depthleft <= 0) {
    score = evaluate(si.currline.cnt);
  } else {

    si.nodes++;

    MoveList movelist = {0};
    genmoves(g_game.who2move, &movelist);

    int legalcnt = 0;

    if (!undercheck() && (si.currline.cnt == 0 ||
                          si.currline.move[si.currline.cnt - 1] != NULLMOVE)) {
      MoveList tmppv;
      makemove(NULLMOVE);
      pushmove(&si.currline, NULLMOVE);
      int nmpscore = -negamax(-beta, -beta + 1, depthleft - 1 - 3, &tmppv);
      popmove(&si.currline);
      unmakemove();

      if (nmpscore >= beta) {
        return nmpscore;
      }
      if (nmpscore > alpha) {
        alpha = nmpscore;
      }
    }

    for (int i = 0; i < movelist.cnt; i++) {
      order(&movelist, i, ttbest);
      makemove(movelist.move[i]);

      if (lastmovelegal()) {
        legalcnt++;

        MoveList subpv = {0};

        if (si.currline.cnt == 0) {
          move2str(si.movestr, movelist.move[i]);
          si.movenum++;
        }

        int ext = 0;

        pushmove(&si.currline, movelist.move[i]);

        if (depthleft == 1 && undercheck())
          ext++;

        score =
            max(score, -negamax(-beta, -alpha, depthleft - 1 + ext, &subpv));
        popmove(&si.currline);

        if (si.currline.cnt == 0) {
          printinfo_regular(score);
        } else {
          // printinfo_regular(score);
        }

        if (score >= beta) {
          unmakemove();
          betacutoff = 1;
          if (IS_SILENT(movelist.move[i]))
            addkiller(si.currline.cnt, movelist.move[i]);
          break;
        }

        if (score > alpha) {
          alpha = score;
          bestmove = movelist.move[i];

          alpharisen = 1;

          pv->move[0] = movelist.move[i];
          for (int j = 0; j < subpv.cnt; j++) {
            pv->move[j + 1] = subpv.move[j];
          }
          pv->cnt = subpv.cnt + 1;
        }
      }

      unmakemove();
    }

    if (legalcnt == 0) {
      score = evaluate(si.currline.cnt);
    }
  }

  int tttype;
  if (betacutoff)
    tttype = TT_BETA;
  else if (alpharisen)
    tttype = TT_EXACT;
  else
    tttype = TT_ALPHA;

  ttwrite(hash, tttype, depthleft, score, bestmove);

  if (si.currline.cnt > si.maxdepth)
    si.maxdepth = si.currline.cnt;

  if (si.currline.cnt == 0) {
    si.bestmove = bestmove;
  }

  return score;
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

static void *search_subthread(void *arg) {
  memset(tt, 0, sizeof(TT) * TT_SIZE);
  for (int depth = 1; depth <= si.requesteddepth; depth++) {
    si.maxdepth = 0;
    si.currline.cnt = 0;
    si.tbhits = 0;
    si.nodes = 0;
    si.movenum = 0;
    memset(&si.killers, 0, sizeof(Move) * KILLER_MAX * MAX_PLY_PER_GAME);
    MoveList pv = {0};
    int score = negamax(SCORE_ILLEGAL, -SCORE_ILLEGAL, depth, &pv);

    printinfo_final(depth, score);
    memcpy(&si.prev_pv, &pv, sizeof(MoveList));
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
