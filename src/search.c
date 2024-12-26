#include <pthread.h>
#include <stdio.h>
#include <unistd.h>

#include "main.h"

#define MIN_PRIORITY (-99999)

typedef struct {
  int movenum;
  char movestr[6];
  int nodes;
  MoveList currline;
  MoveList prev_pv;
  Move bestmove;
  int requesteddepth;
} SearchInfo;

static SearchInfo si;
static pthread_t search_thread;
static pthread_t time_thread;

static int stopping = 0;

int max(int a, int b) {
  if (a > b)
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
    printf("mate %d ", moves * mul);
  } else {
    printf("cp %d ", score);
  }
}

static void printinfo(int score, MoveList *pv) {
  printf("info depth %d nodes %d currmovenumber %d currmove %s ",
         si.currline.cnt, si.nodes, si.movenum, si.movestr);
  printscore(score);
  printf(" currline");
  printline(&si.currline);
  if (pv) {
    printf(" pv");
    printline(pv);
  }
  printf("\n");
}

void reset_hashtables() {}

static int get_priority(Move move) {

  int ply = si.currline.cnt;
  if (ply < si.prev_pv.cnt && move == si.prev_pv.move[ply]) {
    return 99999;
  }

  switch (GET_FLAGS(move)) {
  case MOVE_F_ISCASTLEWQ:
    return 2;
    break;
  case MOVE_F_ISCASTLEBQ:
    return 2;
    break;
  case MOVE_F_ISCASTLEWK:
    return 2;
    break;
  case MOVE_F_ISCASTLEBK:
    return 2;
    break;
  case MOVE_F_ISPROM:
    return 10;
    break;
  case MOVE_F_ISCAPT:
    return 5 + (GET_MOV_PIECE(move) - GET_CAPT_PIECE(move));
    break;
  case MOVE_F_ISPROM | MOVE_F_ISCAPT:
    return 10;
    break;
  case MOVE_F_ISCAPT | MOVE_F_ISEP:
    return 5;
    break;
  case MOVE_F_ISDOUBLEPUSH:
    return 0;
    break;
  case 0:
    return 0;
    break;
  }
  return 0;
}

static void order(MoveList *movelist, int curr) {
  int best_i = -1;
  int best_priority = MIN_PRIORITY;

  for (int i = curr; i < movelist->cnt; i++) {
    int priority = get_priority(movelist->move[i]);
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
  int score = SCORE_ILLEGAL;
  Move bestmove = NULLMOVE;

  if (depthleft == 0) {
    score = evaluate(si.currline.cnt);
  } else {

    si.nodes++;

    MoveList movelist = {0};
    genmoves(g_game.who2move, &movelist);

    int legalcnt = 0;

    for (int i = 0; i < movelist.cnt; i++) {
      order(&movelist, i);
      makemove(movelist.move[i]);

      if (lastmovelegal()) {
        legalcnt++;

        MoveList subpv = {0};

        pushmove(&si.currline, movelist.move[i]);
        score = max(score, -negamax(-beta, -alpha, depthleft - 1, &subpv));
        // printinfo(score, NULL);
        popmove(&si.currline);

        if (score >= beta) {
          unmakemove();
          break;
        }

        if (score > alpha) {
          alpha = score;
          bestmove = movelist.move[i];

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

  if (si.currline.cnt == 0) {
    if (score == SCORE_ILLEGAL) {
      si.bestmove = NULLMOVE;
    } else {
      si.bestmove = bestmove;
    }
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
  for (int depth = 1; depth <= si.requesteddepth; depth++) {
    si.currline.cnt = 0;
    MoveList pv = {0};
    int score = negamax(SCORE_ILLEGAL, -SCORE_ILLEGAL, depth, &pv);
    printinfo(score, &pv);
    printf("info string "
           "_________________________________________________________\n");
    fflush(stdout);
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
