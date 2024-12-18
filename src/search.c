#include <stdio.h>

#include "main.h"

typedef struct {
  int movenum;
  char movestr[6];
  int nodes;
  MoveList currline;
} SearchInfo;

static void printinfo(const SearchInfo *si, int score) {
  printf("info depth %d nodes %d currmovenumber %d currmove %s score cp %d",
         si->currline.cnt, si->nodes, si->movenum, si->movestr, score);
  printf(" currline");
  for (int i = 0; i < si->currline.cnt; i++) {
    char buff[6];
    move2str(buff, si->currline.move[i]);
    printf(" %s", buff);
  }
  printf("\n");
}

void reset_hashtables() {}

int negamax(SearchInfo *si, int alpha, int beta, int depthleft) {
  int playermul = si->currline.cnt % 2 == 0 ? 1 : -1;

  if (depthleft == 0)
    return playermul * evaluate(si->currline.cnt);
  si->nodes++;

  MoveList movelist;
  movelist.cnt = 0;
  genmoves(g_game.who2move, &movelist);

  int legalcnt = 0;

  int bestscore = SCORE_ILLEGAL;
  for (int i = 0; i < movelist.cnt; i++) {
    makemove(movelist.move[i]);

    if (lastmovelegal()) {
      legalcnt++;

      pushmove(&si->currline, movelist.move[i]);
      int score = -negamax(si, -beta, -alpha, depthleft - 1);
      printinfo(si, score);
      popmove(&si->currline);

      if (score > bestscore) {
        bestscore = score;
        if (score > alpha)
          alpha = score;
      }
      /*if (score >= beta)
        {
        unmakemove();
        return bestscore;
        }*/
    }

    unmakemove();
  }

  if (legalcnt == 0) {
    int e = evaluate(si->currline.cnt);

    printf("info string End of search before finishing depth: e %d mul %d\n", e,
           playermul);
    printinfo(si, e);
    return playermul * e;
  }

  return bestscore;
}

void search(int requesteddepth) {
  SearchInfo si = {0};

  if (g_mode == MODE_UCI && g_ucidebug) {
    printf("info string random move\n");
  }

  MoveList movelist;
  genmoves(g_game.who2move, &movelist);

  char movenames[movelist.cnt][6];

  int bestscore = SCORE_ILLEGAL;
  int bestmove = 0;

  int depth = requesteddepth;
  if (requesteddepth == DEPTH_INF)
    depth = 24;

  int legal_i = 1;

  for (int i = 0; i < movelist.cnt; i++) {
    makemove(movelist.move[i]);
    if (lastmovelegal()) {
      si.movenum = legal_i;
      move2str(si.movestr, movelist.move[i]);
      pushmove(&si.currline, movelist.move[i]);

      move2str(movenames[i], movelist.move[i]);

      if (isrepetition()) {
        // printf("info string repetition detected\n");
      }

      int score = -negamax(&si, -SCORE_ILLEGAL, SCORE_ILLEGAL, 1);

      printinfo(&si, score);

      if (score > bestscore) {
        bestscore = score;
        bestmove = i;
        printf("info string new best move\n");
      }

      popmove(&si.currline);
      legal_i++;
    } else {
      char buff[6];
      move2str(buff, movelist.move[i]);
      printf("info string move %s illegal\n", buff);
    }

    unmakemove();
  }

  if (bestscore == SCORE_ILLEGAL) {
    printf("info string no legal move detected\n");
    fflush(stdout);
  } else {
    printf("bestmove %s\n", movenames[bestmove]);
    fflush(stdout);
  }
}
