#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>

#include "main.h"

#define ON_FINISH() (ss.on_finish ? (*ss.on_finish)(&si) : (void)0)
#define ON_ROOTMOVE(score)                                                     \
  (ss.on_rootmove ? (*ss.on_rootmove)(&si, ttused, ttsize, score) : (void)0)
#define ON_NONROOTMOVE(score)                                                  \
  (ss.on_nonrootmove ? (*ss.on_nonrootmove)(&si, ttused, ttsize, score)        \
                     : (void)0)
#define ON_ITERFINISH(score)                                                   \
  (ss.on_iterfinish ? (*ss.on_iterfinish)(&si, ttused, ttsize, score) : (void)0)

#define MIN_PRIORITY (-99999)

#define NODE_INSIDEWND 0
#define NODE_FAILH 1
#define NODE_FAILL 2
#define NODE_GAMEFINISHED 3
#define NODE_SELECTIVELY_PRUNED 4

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
  int searchid;
} TT;

static TT *tt = NULL;
static size_t ttsize;
static size_t ttused = 0;

static SearchInfo si;
static SearchSettings ss;
static pthread_t search_thread;
static pthread_t supervisor_thread;

static int manualstop = 0;
static int abort_search = 0;

static int searchid = 0;

void ttinit() {
  ttsize = g_set.ttbytes / sizeof(TT);
  ttused = 0;
  tt = calloc(ttsize, sizeof(TT));
}

void ttfree() { free(tt); }

static const TT *ttread(BitBrd hash, int depth) {
  TT *ttentry = tt + (hash % ttsize);
  if (ttentry->used && ttentry->hash == hash && ttentry->depth >= depth) {
    ttentry->searchid = searchid;
    return ttentry;
  }
  return NULL;
}

static void ttwrite(BitBrd hash, int type, int depth, int value,
                    Move bestmove) {
  TT *ttentry = tt + (hash % ttsize);

  if (!ttentry->used) {
    ttused++;
  } else if (ttentry->depth > 5 && depth + 2 <= ttentry->depth) {
    if (searchid - ttentry->searchid < 2) {
      return;
    }
  }

  ttentry->used = 1;
  ttentry->hash = hash;
  ttentry->type = type;
  ttentry->depth = depth;
  ttentry->value = value;
  ttentry->bestmove = bestmove;
  ttentry->searchid = searchid;
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

  double seconds = (double)(now - si.nps_lastcalc) / (double)CLOCKS_PER_SEC;
  long nodesdiff = si.iter_visited_nodes; //- si.nps_lastnodes;
                                          //
  if (seconds < 0.001 || nodesdiff < 1) {
    return 0;
  }

  int nps = (int)((double)nodesdiff / seconds);

  // si.nps_lastnodes = si.search_visitednodes;
  // si.nps_lastcalc = clock();

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

static int see(int sqr) {
  MoveList capts;
  BitBrd attackbbrd;
  gen_moves(g_game.who2move, &capts, &attackbbrd, GEN_CAPT, 0);

  Move lva = NULLMOVE;
  for (int i = 0; i < capts.cnt; i++) {
    const Move currmove = capts.move[i];

    if (GET_DST_SQR(currmove) == sqr) {
      if (lva == NULLMOVE ||
          material[GET_MOV_PIECE(currmove)] < material[GET_MOV_PIECE(lva)]) {
        makemove(currmove);
        if (lastmovelegal()) {
          lva = currmove;
        }
        unmakemove();
      }
    }
  }

  if (lva == NULLMOVE) {
    return 0;
  }

  makemove(lva);
  int val = material[GET_CAPT_PIECE(lva)] - see(sqr);
  if (IS_PROM(lva)) {
    val += material[GET_PROM_PIECE(lva)] - 1;
  }
  unmakemove();

  return val > 0 ? val : 0;
}

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

  int diff = 0;
  if (IS_CAPT(move)) {
    int sqr = GET_DST_SQR(move);
    diff = 0;
    makemove(move);
    if (lastmovelegal()) {
      diff = material[GET_CAPT_PIECE(move)] - see(sqr);
      if (IS_PROM(move)) {
        diff += material[GET_PROM_PIECE(move)] - 1;
      }
    }
    unmakemove();
    return captpriority + diff;
  }

  if (IS_PROM(move)) {
    diff += material[GET_PROM_PIECE(move)];
    return captpriority + diff;
  }

  return normalpriority;
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

int quiescence(int alpha, int beta) {
  MoveList availmoves;
  BitBrd attackbbrd;
  gen_moves(g_game.who2move, &availmoves, &attackbbrd, GEN_ALL, 0);

  int standpat;
  if (availmoves.cnt == 0) {
    standpat = evaluate_terminalpos(si.currline.cnt);
  } else {
    standpat = evaluate();
  }

  if (standpat >= beta) {
    return beta;
  }

  int deltaallowed = g_gamestate->phase != PHASE_ENDGAME;

  if (deltaallowed) {
    int delta = material[QUEEN];
    if (standpat < alpha - delta) {
      return alpha;
    }
  }

  si.iter_visited_nodes++;
  si.search_visitednodes++;

  alpha = max(standpat, alpha);

  for (int i = 0; i < availmoves.cnt; i++) {
    order(&availmoves, i, NULLMOVE);
    Move currmove = availmoves.move[i];

    if (!IS_CAPT(currmove)) {
      continue;
    }

    pushmove(&si.currline, currmove);
    makemove(currmove);

    if (lastmovelegal()) {
      int score = -quiescence(-beta, -alpha);
      unmakemove();
      popmove(&si.currline);

      if (score >= beta) {
        return beta;
      }
      alpha = max(score, alpha);
    } else {
      unmakemove();
      popmove(&si.currline);
    }
  }
  return alpha;
}

int negamax(int alpha, int beta, int depthleft, MoveList *pvdest);

void analyze_node(NodeInfo *ni, int depthleft, int *alpha, int beta,
                  Move ttbest, MoveList *pvdest) {
  ni->legalcnt = 0;
  ni->nodetype = NODE_FAILL;
  ni->bestmove = NULLMOVE;
  int score = SCORE_ILLEGAL;

  int stat_eval = evaluate();
  int under_check_cnt = get_under_check_cnt();
  Move last_move = si.currline.move[si.currline.cnt - 1];

  if (!g_set.disbl_nmp && g_gamestate->phase != PHASE_ENDGAME &&
      !possible_zugzwang() && under_check_cnt == 0 && si.currline.cnt != 0 &&
      depthleft > 3 && last_move != NULLMOVE && stat_eval >= beta) {
    makemove(NULLMOVE);
    pushmove(&si.currline, NULLMOVE);
    int nmpscore = -negamax(-beta, -beta + 1, depthleft - 1 - 2, NULL);
    popmove(&si.currline);
    unmakemove();

    if (nmpscore >= beta) {
      ni->nodetype = NODE_SELECTIVELY_PRUNED;
      ni->score = beta;
      return;
    }
  }

  BitBrd attackbbrd;
  gen_moves(g_game.who2move, &ni->availmoves, &attackbbrd, GEN_ALL,
            under_check_cnt);

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

      int movescore = 0;
      int fullsearch = 1;
      int new_under_check_cnt = get_under_check_cnt();

      int pvsallowed =
          i > 0 && !g_set.disbl_pvs && g_gamestate->phase != PHASE_ENDGAME;
      int fpallowed =
          depthleft <= 2 && !IS_CAPT(currmove) && new_under_check_cnt == 0;

      // Extensions
      int ext = 0;
      if (depthleft == 1 && new_under_check_cnt > 0) {
        ext += 1;
      }

      // LMR
      if (!g_set.disbl_lmr && ext == 0 && depthleft > 3 && !IS_CAPT(currmove) &&
          ni->legalcnt > 4) {
        if (IS_SILENT(currmove)) {
          ext -= (int)(0.8 + (log(depthleft) * log(ni->legalcnt - 3) / 2));
        } else {
          ext -= (int)(0.2 + (log(depthleft) * log(ni->legalcnt - 3) / 4));
        }
      }

      // Futility Pruning
      if (fpallowed && ext <= 0) {
        if (-evaluate_material() <
            *alpha - 100 * depthleft + 50 * (depthleft - 1)) {
          movescore = *alpha;
          pvsallowed = 0;
          fullsearch = 0;
        }
      }

      // PVS
      if (pvsallowed) {
        int pvsscore =
            -negamax(-*alpha - 1, -*alpha, depthleft + ext - 1, NULL);

        if (pvsscore <= *alpha) {
          fullsearch = 0;
          movescore = pvsscore;
        }
      }

      MoveList subpv = {0};

      // Full search if required
      if (fullsearch) {
        movescore = -negamax(-beta, -*alpha, depthleft + ext - 1, &subpv);

        if (ext < 0 && movescore > *alpha) {
          // reduced move raised alpha
          // re-search without reductions
          subpv.cnt = 0;
          movescore = -negamax(-beta, -*alpha, depthleft - 1, &subpv);
        }
      }

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
        ni->score = score;
        return;
      }

      if (score > *alpha) {
        ni->nodetype = NODE_INSIDEWND;
        ni->bestmove = currmove;
        if (pvdest != NULL) {
          pvdest->cnt = 1;
          pvdest->move[0] = currmove;
          for (int m = 0; m < subpv.cnt; m++) {
            if (subpv.move[m] == NULLMOVE) {
              break;
            }
            pushmove(pvdest, subpv.move[m]);
          }
        }
        *alpha = score;
      }
    }

    unmakemove();
    popmove(&si.currline);
  }

  if (ni->legalcnt == 0) {
    ni->nodetype = NODE_GAMEFINISHED;
    ni->score = evaluate_terminalpos(si.currline.cnt);
  } else {
    ni->score = score;
  }
}

int negamax(int alpha, int beta, int depthleft, MoveList *pvdest) {
  if (pvdest)
    pvdest->cnt = 0;

  const BitBrd hash = g_gamestate->hash;

  if (abort_search) {
    return SCORE_ILLEGAL;
  }

  if (si.root_repetitions >= 3) {
    return 0;
  }

  if (si.currline.cnt > 0) {
    int rep = getrepetitions();
    if (rep > 0)
      return 0;
  }

  if (g_gamestate->halfmove >= 100) {
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

  si.iter_visited_nodes++;
  si.search_visitednodes++;

  if (si.currline.cnt > si.iter_highest_depth) {
    si.iter_highest_depth = si.currline.cnt;
  }

  if (depthleft <= 0) {
    return quiescence(alpha, beta);
  }

  NodeInfo ni;
  analyze_node(&ni, depthleft, &alpha, beta, ttbest, pvdest);

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

static void recoverpv(MoveList *pv) {
  int pvcnt = pv->cnt;

  for (int i = 0; i < pvcnt; i++) {
    makemove(pv->move[i]);
  }

  const TT *ttentry = ttread(g_gamestate->hash, 1);

  if (ttentry && ttentry->hash == g_gamestate->hash &&
      ttentry->bestmove != NULLMOVE) {
    makemove(ttentry->bestmove);
    if (g_gamestate->halfmove < 100 && getrepetitions() < 3) {
      pushmove(pv, ttentry->bestmove);
      recoverpv(pv);
    }
    unmakemove();
  }

  for (int i = 0; i < pvcnt; i++) {
    unmakemove();
  }
}

static void *search_subthread(void *arg __attribute__((unused))) {
  int firsttime = 1;
  int lastscore = SCORE_ILLEGAL;

  si.search_visitednodes = 0;
  for (int depth = ss.startdepth; depth <= ss.depthlimit; depth++) {
    searchid++;
    si.root_repetitions = getrepetitions();
    si.iter_highest_depth = 0;
    si.currline.cnt = 0;
    si.iter_tbhits = 0;
    si.iter_visited_nodes = 0;
    si.nps_lastnodes = 0;
    si.nps_lastcalc = clock();
    si.rootmove_n = 0;
    si.iter_depth = depth;
    memset(&si.iter_killers, 0, sizeof(Move) * KILLER_MAX * MAX_PLY_PER_GAME);

    MoveList pv = {0};
    int score;
    if (g_set.disbl_aspwnd || firsttime || IS_CHECKMATE(lastscore)) {
      score = negamax(SCORE_ILLEGAL, -SCORE_ILLEGAL, depth, &pv);
      firsttime = 0;

      if (abort_search) {
        return 0;
      }
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

        score = negamax(alpha, beta, depth, &pv);

        if (abort_search) {
          return 0;
        }

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
    // recoverpv(&pv);

    memcpy(&si.prev_iter_pv, &pv, sizeof(MoveList));
    ON_ITERFINISH(score);

    if (IS_CHECKMATE(score)) {
      break;
    }
  }

  si.finished = 1;
  return 0;
}

static void *supervisor_subthread(void *arg __attribute__((unused))) {
  clock_t start_time = clock() / CLOCKS_PER_MS;

  pthread_create(&search_thread, NULL, search_subthread, NULL);

  while (1) {
    if (si.finished) {
      break;
    }
    // todo PRINTDBG needs lock
    if (ss.timelimit != TIME_FOREVER &&
        difftime(clock() / CLOCKS_PER_MS, start_time) >= ss.timelimit) {
      // PRINTDBG("cancelling on time");
      break;
    }
    if (ss.nodeslimit != 0 && si.iter_visited_nodes >= ss.nodeslimit) {
      // PRINTDBG("canceling on nodes limit");
      break;
    }
    if (manualstop) {
      // PRINTDBG("canceling on time");
      break;
    }

    usleep(1000);
  }
  fflush(stdout);

  while (si.prev_iter_pv.cnt == 0) {
    usleep(1000);
  }

  // pthread_cancel(search_thread);
  abort_search = 1;
  pthread_join(search_thread, NULL);

  search_finish();
  return 0;
}

void search(const SearchSettings *settings) {
  memset(&si, 0, sizeof(SearchInfo));

  memcpy(&ss, settings, sizeof(SearchSettings));

  manualstop = 0;
  abort_search = 0;
  si.finished = 0;
  pthread_create(&supervisor_thread, NULL, supervisor_subthread, NULL);
}

void stop() {
  PRINTDBG("canceling manually");
  fflush(stdout);
  manualstop = 1;
  pthread_join(supervisor_thread, NULL);
}
