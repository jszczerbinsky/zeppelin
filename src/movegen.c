#include "main.h"

#define GEN_ALL 0
#define GEN_QUIET 1
#define GEN_CAPT 2

int sqr_attackedby(int attacker, int sqr) {
  BitBrd occ, index;

  if (g_game.pieces[attacker][PAWN] & g_precomp.pawnattackmask[!attacker][sqr])
    return 1;

  if (g_game.pieces[attacker][KNIGHT] & g_precomp.knightmask[sqr])
    return 1;

  if (g_game.pieces[attacker][KING] & g_precomp.kingmask[sqr])
    return 1;

  occ = g_game.piecesof[ANY] & g_precomp.bishoppremask[sqr];
  index = (occ * g_precomp.bishopmagic[sqr]) >> g_precomp.bishopmagicshift[sqr];

  if ((g_game.pieces[attacker][BISHOP] | g_game.pieces[attacker][QUEEN]) &
      g_precomp.bishopmagicmoves[sqr][index])
    return 1;

  occ = g_game.piecesof[ANY] & g_precomp.rookpremask[sqr];
  index = (occ * g_precomp.rookmagic[sqr]) >> g_precomp.rookmagicshift[sqr];

  if ((g_game.pieces[attacker][ROOK] | g_game.pieces[attacker][QUEEN]) &
      g_precomp.rookmagicmoves[sqr][index])
    return 1;

  return 0;
}

static void gen_sliding(int player, MoveList *movelist, int piece, int type) {
  BitBrd piecesbbrd = g_game.pieces[player][piece];

  while (piecesbbrd) {
    int piecesqr = bbrd2sqr(piecesbbrd);
    BitBrd poss_dstbbrd = 0ULL;

    if (piece == ROOK || piece == QUEEN) {
      BitBrd occ = g_game.piecesof[ANY] & g_precomp.rookpremask[piecesqr];
      BitBrd index = (occ * g_precomp.rookmagic[piecesqr]) >>
                     g_precomp.rookmagicshift[piecesqr];
      poss_dstbbrd |= g_precomp.rookmagicmoves[piecesqr][index];
    }
    if (piece == BISHOP || piece == QUEEN) {
      BitBrd occ = g_game.piecesof[ANY] & g_precomp.bishoppremask[piecesqr];
      BitBrd index = (occ * g_precomp.bishopmagic[piecesqr]) >>
                     g_precomp.bishopmagicshift[piecesqr];
      poss_dstbbrd |= g_precomp.bishopmagicmoves[piecesqr][index];
    }

    poss_dstbbrd &= ~g_game.piecesof[player];

    while (poss_dstbbrd) {
      int dstsqr = bbrd2sqr(poss_dstbbrd);
      BitBrd dstbbrd = sqr2bbrd(dstsqr);

      Move move = SRC_SQR(piecesqr) | DST_SQR(dstsqr) | MOV_PIECE(piece);

      if (dstbbrd & g_game.piecesof[!player]) {
        if (type != GEN_QUIET) {
          move |=
              MOVE_TYPE_NORMALCAPT | CAPT_PIECE(getpieceat(!player, dstbbrd));
          pushmove(movelist, move);
        }
      } else if (type != GEN_CAPT) {
        move |= MOVE_TYPE_NORMAL;
        pushmove(movelist, move);
      }

      poss_dstbbrd &= ~dstbbrd;
    }

    piecesbbrd &= ~sqr2bbrd(piecesqr);
  }
}

static void gen_knight(int player, MoveList *movelist, int type) {
  BitBrd knightsbbrd = g_game.pieces[player][KNIGHT];

  while (knightsbbrd) {
    int knightsqr = bbrd2sqr(knightsbbrd);

    BitBrd poss_dstbbrd =
        g_precomp.knightmask[knightsqr] & (~g_game.piecesof[player]);

    while (poss_dstbbrd) {
      int dstsqr = bbrd2sqr(poss_dstbbrd);
      BitBrd dstbbrd = sqr2bbrd(dstsqr);

      Move move = SRC_SQR(knightsqr) | DST_SQR(dstsqr) | MOV_PIECE(KNIGHT);

      if (dstbbrd & g_game.piecesof[!player]) {
        if (type != GEN_QUIET) {
          move |=
              MOVE_TYPE_NORMALCAPT | CAPT_PIECE(getpieceat(!player, dstbbrd));
          pushmove(movelist, move);
        }
      } else if (type != GEN_CAPT) {
        move |= MOVE_TYPE_NORMAL;
        pushmove(movelist, move);
      }

      poss_dstbbrd &= ~dstbbrd;
    }

    knightsbbrd &= ~sqr2bbrd(knightsqr);
  }
}

static void gen_king(int player, MoveList *movelist, int type) {
  int kingsqr = bbrd2sqr(g_game.pieces[player][KING]);

  BitBrd poss_dstbbrd =
      g_precomp.kingmask[kingsqr] & (~g_game.piecesof[player]);

  while (poss_dstbbrd) {
    int dstsqr = bbrd2sqr(poss_dstbbrd);
    BitBrd dstbbrd = sqr2bbrd(dstsqr);

    Move move = SRC_SQR(kingsqr) | DST_SQR(dstsqr) | MOV_PIECE(KING);

    if (dstbbrd & g_game.piecesof[!player]) {
      if (type != GEN_QUIET) {
        move |= MOVE_TYPE_NORMALCAPT | CAPT_PIECE(getpieceat(!player, dstbbrd));
        pushmove(movelist, move);
      }
    } else if (type != GEN_CAPT) {
      move |= MOVE_TYPE_NORMAL;
      pushmove(movelist, move);
    }

    poss_dstbbrd &= ~dstbbrd;
  }
}

static void gen_castle(int player, MoveList *movelist) {
  if (player == WHITE && !sqr_attackedby(BLACK, W_KINGSQR)) {
    if (CANCASTLE_WK(g_gamestate) && (g_game.piecesof[ANY] & 0x60ULL) == 0 &&
        !sqr_attackedby(BLACK, W_KINGSQR + 1) &&
        !sqr_attackedby(BLACK, W_KINGSQR + 2))
      pushmove(movelist, MOVE_TYPE_CASTLEWK);
    if (CANCASTLE_WQ(g_gamestate) && (g_game.piecesof[ANY] & 0xeULL) == 0 &&
        !sqr_attackedby(BLACK, W_KINGSQR - 1) &&
        !sqr_attackedby(BLACK, W_KINGSQR - 2))
      pushmove(movelist, MOVE_TYPE_CASTLEWQ);
  } else if (player == BLACK && !sqr_attackedby(WHITE, B_KINGSQR)) {
    if (CANCASTLE_BK(g_gamestate) &&
        (g_game.piecesof[ANY] & 0x6000000000000000ULL) == 0 &&
        !sqr_attackedby(WHITE, B_KINGSQR + 1) &&
        !sqr_attackedby(WHITE, B_KINGSQR + 2))
      pushmove(movelist, MOVE_TYPE_CASTLEBK);
    if (CANCASTLE_BQ(g_gamestate) &&
        (g_game.piecesof[ANY] & 0xe00000000000000ULL) == 0 &&
        !sqr_attackedby(WHITE, B_KINGSQR - 1) &&
        !sqr_attackedby(WHITE, B_KINGSQR - 2))
      pushmove(movelist, MOVE_TYPE_CASTLEBQ);
  }
}

static void gen_pawncapt(int player, MoveList *movelist) {
  const BitBrd player_pawns = g_game.pieces[player][PAWN];

  BitBrd poss_l_dstbbrd, poss_r_dstbbrd;

  if (player == WHITE) {
    poss_l_dstbbrd = ((player_pawns & (~FILE_A)) << 7) &
                     (g_game.piecesof[!player] | g_gamestate->epbbrd);
    poss_r_dstbbrd = ((player_pawns & (~FILE_H)) << 9) &
                     (g_game.piecesof[!player] | g_gamestate->epbbrd);
  } else {
    poss_l_dstbbrd = ((player_pawns & (~FILE_H)) >> 7) &
                     (g_game.piecesof[!player] | g_gamestate->epbbrd);
    poss_r_dstbbrd = ((player_pawns & (~FILE_A)) >> 9) &
                     (g_game.piecesof[!player] | g_gamestate->epbbrd);
  }

  while (poss_l_dstbbrd) {
    int dstsqr = bbrd2sqr(poss_l_dstbbrd);
    BitBrd dstbbrd = sqr2bbrd(dstsqr);
    int srcsqr;

    const int isprom = (player == WHITE && (dstbbrd & RANK_8)) ||
                       (player == BLACK && (dstbbrd & RANK_1));

    if (player == WHITE)
      srcsqr = dstsqr - 7;
    else
      srcsqr = dstsqr + 7;

    Move move = SRC_SQR(srcsqr) | DST_SQR(dstsqr) | MOV_PIECE(PAWN);

    if (dstbbrd & g_gamestate->epbbrd) {
      move |= MOVE_TYPE_EP | CAPT_PIECE(PAWN);
    } else {
      move |= CAPT_PIECE(getpieceat(!player, dstbbrd));
      if (isprom) {
        move |= MOVE_TYPE_PROMCAPT;
      } else {
        move |= MOVE_TYPE_NORMALCAPT;
      }
    }

    if (isprom)
      pushprommove(movelist, move);
    else
      pushmove(movelist, move);

    poss_l_dstbbrd &= ~dstbbrd;
  }

  while (poss_r_dstbbrd) {
    int dstsqr = bbrd2sqr(poss_r_dstbbrd);
    BitBrd dstbbrd = sqr2bbrd(dstsqr);
    int srcsqr;

    const int isprom = (player == WHITE && (dstbbrd & RANK_8)) ||
                       (player == BLACK && (dstbbrd & RANK_1));

    if (player == WHITE)
      srcsqr = dstsqr - 9;
    else
      srcsqr = dstsqr + 9;

    Move move = SRC_SQR(srcsqr) | DST_SQR(dstsqr) | MOV_PIECE(PAWN);

    if (dstbbrd & g_gamestate->epbbrd) {
      move |= MOVE_TYPE_EP | CAPT_PIECE(PAWN);
    } else {
      move |= CAPT_PIECE(getpieceat(!player, dstbbrd));
      if (isprom) {
        move |= MOVE_TYPE_PROMCAPT;
      } else {
        move |= MOVE_TYPE_NORMALCAPT;
      }
    }

    if (isprom)
      pushprommove(movelist, move);
    else
      pushmove(movelist, move);

    poss_r_dstbbrd &= ~dstbbrd;
  }
}

static void gen_pushprom(int player, MoveList *movelist) {
  const BitBrd player_pawns = g_game.pieces[player][PAWN];

  BitBrd dstbbrd;

  if (player == WHITE)
    dstbbrd = (player_pawns << 8) & (~g_game.piecesof[ANY]) & RANK_8;
  else
    dstbbrd = (player_pawns >> 8) & (~g_game.piecesof[ANY]) & RANK_1;

  while (dstbbrd) {
    int dstsqr = bbrd2sqr(dstbbrd);
    int srcsqr;

    if (player == WHITE)
      srcsqr = dstsqr - 8;
    else
      srcsqr = dstsqr + 8;

    Move move = SRC_SQR(srcsqr) | DST_SQR(dstsqr) | MOV_PIECE(PAWN) |
                MOVE_TYPE_NORMALPROM;
    pushprommove(movelist, move);

    dstbbrd &= ~sqr2bbrd(dstsqr);
  }
}

static void gen_pawnsilent(int player, MoveList *movelist) {
  BitBrd single_dstbbrd, double_dstbbrd;

  if (player == WHITE) {
    single_dstbbrd = (g_game.pieces[player][PAWN] << 8) &
                     (~g_game.piecesof[ANY]) & (~RANK_8);
    double_dstbbrd = ((single_dstbbrd & RANK_3) << 8) & (~g_game.piecesof[ANY]);
  } else {
    single_dstbbrd = (g_game.pieces[player][PAWN] >> 8) &
                     (~g_game.piecesof[ANY]) & (~RANK_1);
    double_dstbbrd = ((single_dstbbrd & RANK_6) >> 8) & (~g_game.piecesof[ANY]);
  }

  while (single_dstbbrd) {
    int dstsqr = bbrd2sqr(single_dstbbrd);
    int srcsqr;

    if (player == WHITE)
      srcsqr = dstsqr - 8;
    else
      srcsqr = dstsqr + 8;

    Move move =
        SRC_SQR(srcsqr) | DST_SQR(dstsqr) | MOV_PIECE(PAWN) | MOVE_TYPE_NORMAL;
    pushmove(movelist, move);

    single_dstbbrd &= ~sqr2bbrd(dstsqr);
  }

  while (double_dstbbrd) {
    int dstsqr = bbrd2sqr(double_dstbbrd);
    int srcsqr;

    if (player == WHITE)
      srcsqr = dstsqr - 16;
    else
      srcsqr = dstsqr + 16;

    Move move = SRC_SQR(srcsqr) | DST_SQR(dstsqr) | MOV_PIECE(PAWN) |
                MOVE_TYPE_DOUBLEPUSH;
    pushmove(movelist, move);

    double_dstbbrd &= ~sqr2bbrd(dstsqr);
  }
}

void genmoves(int player, MoveList *movelist) {
  movelist->cnt = 0;
  gen_pawnsilent(player, movelist);
  gen_pushprom(player, movelist);
  gen_pawncapt(player, movelist);
  gen_king(player, movelist, GEN_ALL);
  gen_castle(player, movelist);
  gen_knight(player, movelist, GEN_ALL);
  gen_sliding(player, movelist, ROOK, GEN_ALL);
  gen_sliding(player, movelist, BISHOP, GEN_ALL);
  gen_sliding(player, movelist, QUEEN, GEN_ALL);
}

void genquiet(int player, MoveList *movelist) {
  movelist->cnt = 0;
  gen_pawnsilent(player, movelist);
  gen_king(player, movelist, GEN_QUIET);
  gen_castle(player, movelist);
  gen_knight(player, movelist, GEN_QUIET);
  gen_sliding(player, movelist, ROOK, GEN_QUIET);
  gen_sliding(player, movelist, BISHOP, GEN_QUIET);
  gen_sliding(player, movelist, QUEEN, GEN_QUIET);
}

void gencapt(int player, MoveList *movelist) {
  movelist->cnt = 0;
  gen_pushprom(player, movelist);
  gen_pawncapt(player, movelist);
  gen_king(player, movelist, GEN_CAPT);
  gen_knight(player, movelist, GEN_CAPT);
  gen_sliding(player, movelist, ROOK, GEN_CAPT);
  gen_sliding(player, movelist, BISHOP, GEN_CAPT);
  gen_sliding(player, movelist, QUEEN, GEN_CAPT);
}
