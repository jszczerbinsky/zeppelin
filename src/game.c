#include <stdio.h>
#include <string.h>

#include "main.h"

#define FEN_TOK_DELIMS " /\n"

Game g_game;
GameState *g_gamestate;

static inline char *fen_nexttok() { return strtok(NULL, FEN_TOK_DELIMS); }

static inline void update_gamestate() {
  g_gamestate = g_game.brdstate + g_game.movelist.cnt;
}

void init_bbrds() {
  for (int p = 0; p < 2; p++) {
    g_game.piecesof[p] = 0;
    for (int i = 0; i < PIECE_MAX; i++) {
      g_game.piecesof[p] |= g_game.pieces[p][i];
    }
  }

  for (int i = 0; i < PIECE_MAX; i++)
    g_game.pieces[ANY][i] = g_game.pieces[WHITE][i] | g_game.pieces[BLACK][i];

  g_game.piecesof[ANY] = g_game.piecesof[WHITE] | g_game.piecesof[BLACK];

  update_gamestate();
}

int getpieceat(int color, BitBrd bbrd) {
  for (int p = 0; p < PIECE_MAX; p++)
    if (g_game.pieces[color][p] & bbrd)
      return p;
  return -1;
}

void reset_game() {
  g_game.who2move = WHITE;
  g_game.movelist.cnt = 0;

  g_game.brdstate[0].halfmove = 0;
  g_game.brdstate[0].fullmove = 0;
  g_game.brdstate[0].flags = GAME_F_DEFAULT;

  g_game.pieces[WHITE][PAWN] = 0xff00ULL;
  g_game.pieces[WHITE][KING] = 0x10ULL;
  g_game.pieces[WHITE][KNIGHT] = 0x42ULL;
  g_game.pieces[WHITE][BISHOP] = 0x24ULL;
  g_game.pieces[WHITE][ROOK] = 0x81ULL;
  g_game.pieces[WHITE][QUEEN] = 0x8ULL;

  g_game.pieces[BLACK][PAWN] = 0xff000000000000ULL;
  g_game.pieces[BLACK][KING] = 0x1000000000000000ULL;
  g_game.pieces[BLACK][KNIGHT] = 0x4200000000000000ULL;
  g_game.pieces[BLACK][BISHOP] = 0x2400000000000000ULL;
  g_game.pieces[BLACK][ROOK] = 0x8100000000000000ULL;
  g_game.pieces[BLACK][QUEEN] = 0x800000000000000ULL;

  init_bbrds();
  g_gamestate->hash = gethash();
  g_gamestate->phase = PHASE_OPENING;
}

char *parsefen(char *fen) {
  memset(&g_game, 0, sizeof(Game));
  update_gamestate();

  char *token = strtok(fen, FEN_TOK_DELIMS);

  if (!token) {
    return 0;
  }

  int rank = 7;

  while (rank >= 0) {
    int file = 0;
    while (file < 8) {
      if (!token) {
        return 0;
      }

      int sqr = rank * 8 + file;
      if (*token >= '1' && *token <= '8') {
        file += *token - '0';
        token++;
      } else {
        switch (*token) {
        case 'p':
          g_game.pieces[BLACK][PAWN] |= sqr2bbrd(sqr);
          break;
        case 'k':
          g_game.pieces[BLACK][KING] |= sqr2bbrd(sqr);
          break;
        case 'n':
          g_game.pieces[BLACK][KNIGHT] |= sqr2bbrd(sqr);
          break;
        case 'b':
          g_game.pieces[BLACK][BISHOP] |= sqr2bbrd(sqr);
          break;
        case 'r':
          g_game.pieces[BLACK][ROOK] |= sqr2bbrd(sqr);
          break;
        case 'q':
          g_game.pieces[BLACK][QUEEN] |= sqr2bbrd(sqr);
          break;
        case 'P':
          g_game.pieces[WHITE][PAWN] |= sqr2bbrd(sqr);
          break;
        case 'K':
          g_game.pieces[WHITE][KING] |= sqr2bbrd(sqr);
          break;
        case 'N':
          g_game.pieces[WHITE][KNIGHT] |= sqr2bbrd(sqr);
          break;
        case 'B':
          g_game.pieces[WHITE][BISHOP] |= sqr2bbrd(sqr);
          break;
        case 'R':
          g_game.pieces[WHITE][ROOK] |= sqr2bbrd(sqr);
          break;
        case 'Q':
          g_game.pieces[WHITE][QUEEN] |= sqr2bbrd(sqr);
          break;
        default:
          break;
        }
        token++;
        file++;
      }
    }
    rank--;
    token = fen_nexttok();
  }

  if (!token) {
    return 0;
  }

  if (token[0] == 'w')
    g_game.who2move = WHITE;
  else if (token[0] == 'b')
    g_game.who2move = BLACK;

  token = fen_nexttok();
  if (!token) {
    return 0;
  }

  while (*token && *token != '-') {
    switch (*token) {
    case 'k':
      g_gamestate->flags |= GAME_F_CANCASTLE_BK;
      break;
    case 'q':
      g_gamestate->flags |= GAME_F_CANCASTLE_BQ;
      break;
    case 'K':
      g_gamestate->flags |= GAME_F_CANCASTLE_WK;
      break;
    case 'Q':
      g_gamestate->flags |= GAME_F_CANCASTLE_WQ;
      break;
    default:
      break;
    }
    token++;
  }

  token = fen_nexttok();
  if (!token) {
    return 0;
  }

  if (*token != '-') {
    int file = token[0] - 'a';
    int rank = token[1] - '1';

    g_gamestate->epbbrd = sqr2bbrd(file + rank * 8);
  }

  token = fen_nexttok();
  if (!token) {
    return 0;
  }

  int halfmov = 0;
  while (*token) {
    if (*token < '0' || *token > '9')
      break;

    halfmov *= 10;
    halfmov += *token - '0';
    token++;
  }
  g_gamestate->halfmove = halfmov;

  token = fen_nexttok();
  if (!token) {
    return 0;
  }

  int fullmov = 0;
  while (*token) {
    if (*token < '0' || *token > '9')
      break;

    fullmov *= 10;
    fullmov += *token - '0';
    token++;
  }
  g_gamestate->fullmove = fullmov;
  g_gamestate->hash = gethash();

  if (popcnt(g_game.piecesof[ANY]) <= 6) {
    g_gamestate->phase = PHASE_ENDGAME;
  } else if (g_gamestate->fullmove <= 10) {
    g_gamestate->phase = PHASE_OPENING;
  } else {
    g_gamestate->phase = PHASE_MIDDLEGAME;
  }

  init_bbrds();

  return fen_nexttok();
}

Move parsemove(const char *str) {
  int srcsqr = (str[0] - 'a') + (str[1] - '1') * 8;
  int dstsqr = (str[2] - 'a') + (str[3] - '1') * 8;

  BitBrd srcbbrd = sqr2bbrd(srcsqr);
  BitBrd dstbbrd = sqr2bbrd(dstsqr);

  const int enemy = !g_game.who2move;
  const int movpiece = getpieceat(g_game.who2move, srcbbrd);

  if (movpiece == KING) {
    if (dstsqr == 2 && g_game.who2move == WHITE && CANCASTLE_WQ(g_gamestate))
      return MOVE_TYPE_CASTLEWQ;
    if (dstsqr == 6 && g_game.who2move == WHITE && CANCASTLE_WK(g_gamestate))
      return MOVE_TYPE_CASTLEWK;
    if (dstsqr == 58 && g_game.who2move == BLACK && CANCASTLE_BQ(g_gamestate))
      return MOVE_TYPE_CASTLEBQ;
    if (dstsqr == 62 && g_game.who2move == BLACK && CANCASTLE_BK(g_gamestate))
      return MOVE_TYPE_CASTLEBK;
  }

  Move move = SRC_SQR(srcsqr) | DST_SQR(dstsqr) | MOV_PIECE(movpiece);

  int isprom = 1;

  switch (str[4]) {
  case 'q':
    move |= PROM_PIECE(QUEEN);
    break;
  case 'b':
    move |= PROM_PIECE(BISHOP);
    break;
  case 'r':
    move |= PROM_PIECE(ROOK);
    break;
  case 'n':
    move |= PROM_PIECE(KNIGHT);
    break;
  default:
    isprom = 0;
    break;
  }

  if (g_game.piecesof[enemy] & dstbbrd) {
    int piece = getpieceat(enemy, dstbbrd);
    move |= CAPT_PIECE(piece);
    if (isprom) {
      move |= MOVE_TYPE_PROMCAPT;
    } else {
      move |= MOVE_TYPE_NORMALCAPT;
    }
  } else if (movpiece == PAWN) {
    if (g_gamestate->epbbrd & dstbbrd) {
      move |= MOVE_TYPE_EP | CAPT_PIECE(PAWN);
    } else if ((sqr2bbrd(srcsqr) & (RANK_2 | RANK_7)) &&
               (sqr2bbrd(dstsqr) & (RANK_4 | RANK_5))) {
      move |= MOVE_TYPE_DOUBLEPUSH;
    }
  }

  if (GET_TYPE(move) == 0) {
    if (isprom) {
      move |= MOVE_TYPE_NORMALPROM;
    } else {
      move |= MOVE_TYPE_NORMAL;
    }
  }

  return move;
}

void move2str(char *buff, Move move) {
  if (move == NULLMOVE) {
    strcpy(buff, "null");
    return;
  }

  if (IS_CASTLE(move)) {
    switch (GET_TYPE(move)) {
    case MOVE_TYPE_CASTLEWK:
      strcpy(buff, "e1g1");
      return;
    case MOVE_TYPE_CASTLEBK:
      strcpy(buff, "e8g8");
      return;
    case MOVE_TYPE_CASTLEWQ:
      strcpy(buff, "e1c1");
      return;
    case MOVE_TYPE_CASTLEBQ:
      strcpy(buff, "e8c8");
      return;
    }
  }

  int srcsqr = GET_SRC_SQR(move);
  int dstsqr = GET_DST_SQR(move);

  buff[0] = (srcsqr % 8) + 'a';
  buff[1] = (srcsqr / 8) + '1';
  buff[2] = (dstsqr % 8) + 'a';
  buff[3] = (dstsqr / 8) + '1';

  if (IS_PROM(move)) {
    switch (GET_PROM_PIECE(move)) {
    case BISHOP:
      buff[4] = 'b';
      break;
    case ROOK:
      buff[4] = 'r';
      break;
    case KNIGHT:
      buff[4] = 'n';
      break;
    case QUEEN:
      buff[4] = 'q';
      break;
    default:
      break;
    }
    buff[5] = '\0';
  } else
    buff[4] = '\0';
}

int getrepetitions() {
  int count = 0;
  for (int i = 0; i < g_game.movelist.cnt; i++) {
    if (g_gamestate->hash == g_game.brdstate[i].hash) {
      count++;
    }
  }
  return count;
}

#define removepiece(color, piece, bbrd)                                        \
  {                                                                            \
    g_game.pieces[color][piece] &= ~(bbrd);                                    \
    g_game.pieces[ANY][piece] &= ~(bbrd);                                      \
    g_game.piecesof[color] &= ~(bbrd);                                         \
    g_game.piecesof[ANY] &= ~(bbrd);                                           \
  }

#define putpiece(color, piece, bbrd)                                           \
  {                                                                            \
    g_game.pieces[color][piece] |= bbrd;                                       \
    g_game.pieces[ANY][piece] |= bbrd;                                         \
    g_game.piecesof[color] |= bbrd;                                            \
    g_game.piecesof[ANY] |= bbrd;                                              \
  }

void makemove(Move move) {
  GameState *newgamestate = g_gamestate + 1;
  memcpy(newgamestate, g_gamestate, sizeof(GameState));

  const int player = g_game.who2move;
  const int srcsqr = GET_SRC_SQR(move);
  const int dstsqr = GET_DST_SQR(move);
  const BitBrd srcbbrd = sqr2bbrd(srcsqr);
  const BitBrd dstbbrd = sqr2bbrd(dstsqr);

  newgamestate->epbbrd = 0;
  newgamestate->halfmove++;
  newgamestate->hash = g_gamestate->hash ^ hash_whitemove;
  if (g_gamestate->epbbrd != 0) {
    newgamestate->hash ^= hash_epfile[bbrd2sqr(g_gamestate->epbbrd) % 8];
  }

  if (player == BLACK) {
    newgamestate->fullmove++;
  }

  if (GET_MOV_PIECE(move) == ROOK) {
    switch (GET_SRC_SQR(move)) {
    case WK_ROOKSQR:
      newgamestate->flags &= ~GAME_F_CANCASTLE_WK;
      if (CANCASTLE_WK(g_gamestate)) {
        newgamestate->hash ^= hash_castle_wk;
      }
      break;
    case BK_ROOKSQR:
      newgamestate->flags &= ~GAME_F_CANCASTLE_BK;
      if (CANCASTLE_BK(g_gamestate)) {
        newgamestate->hash ^= hash_castle_bk;
      }
      break;
    case WQ_ROOKSQR:
      newgamestate->flags &= ~GAME_F_CANCASTLE_WQ;
      if (CANCASTLE_WQ(g_gamestate)) {
        newgamestate->hash ^= hash_castle_wq;
      }
      break;
    case BQ_ROOKSQR:
      newgamestate->flags &= ~GAME_F_CANCASTLE_BQ;
      if (CANCASTLE_BQ(g_gamestate)) {
        newgamestate->hash ^= hash_castle_bq;
      }
      break;
    default:
      break;
    }
  } else if (GET_MOV_PIECE(move) == KING) {
    if (player == WHITE) {
      newgamestate->flags &= ~(GAME_F_CANCASTLE_WQ | GAME_F_CANCASTLE_WK);
      if (CANCASTLE_WK(g_gamestate)) {
        newgamestate->hash ^= hash_castle_wk;
      }
      if (CANCASTLE_WQ(g_gamestate)) {
        newgamestate->hash ^= hash_castle_wq;
      }
    } else {
      newgamestate->flags &= ~(GAME_F_CANCASTLE_BQ | GAME_F_CANCASTLE_BK);
      if (CANCASTLE_BK(g_gamestate)) {
        newgamestate->hash ^= hash_castle_bk;
      }
      if (CANCASTLE_BQ(g_gamestate)) {
        newgamestate->hash ^= hash_castle_bq;
      }
    }
  }

  switch (GET_TYPE(move)) {
  case MOVE_TYPE_CASTLEWQ:
    removepiece(WHITE, KING, sqr2bbrd(W_KINGSQR));
    putpiece(WHITE, KING, sqr2bbrd(CASTLE_WQ_KINGSQR));
    removepiece(WHITE, ROOK, sqr2bbrd(WQ_ROOKSQR));
    putpiece(WHITE, ROOK, sqr2bbrd(CASTLE_WQ_ROOKSQR));

    newgamestate->hash ^= hash_piecesqr[player][W_KINGSQR][KING];
    newgamestate->hash ^= hash_piecesqr[player][CASTLE_WQ_KINGSQR][KING];
    newgamestate->hash ^= hash_piecesqr[player][WQ_ROOKSQR][ROOK];
    newgamestate->hash ^= hash_piecesqr[player][CASTLE_WQ_ROOKSQR][ROOK];

    newgamestate->flags &= ~(GAME_F_CANCASTLE_WK | GAME_F_CANCASTLE_WQ);
    if (CANCASTLE_WK(g_gamestate)) {
      newgamestate->hash ^= hash_castle_wk;
    }
    if (CANCASTLE_WQ(g_gamestate)) {
      newgamestate->hash ^= hash_castle_wq;
    }
    break;
  case MOVE_TYPE_CASTLEBQ:
    removepiece(BLACK, KING, sqr2bbrd(B_KINGSQR));
    putpiece(BLACK, KING, sqr2bbrd(CASTLE_BQ_KINGSQR));
    removepiece(BLACK, ROOK, sqr2bbrd(BQ_ROOKSQR));
    putpiece(BLACK, ROOK, sqr2bbrd(CASTLE_BQ_ROOKSQR));

    newgamestate->hash ^= hash_piecesqr[player][B_KINGSQR][KING];
    newgamestate->hash ^= hash_piecesqr[player][CASTLE_BQ_KINGSQR][KING];
    newgamestate->hash ^= hash_piecesqr[player][BQ_ROOKSQR][ROOK];
    newgamestate->hash ^= hash_piecesqr[player][CASTLE_BQ_ROOKSQR][ROOK];

    newgamestate->flags &= ~(GAME_F_CANCASTLE_BK | GAME_F_CANCASTLE_BQ);
    if (CANCASTLE_BK(g_gamestate)) {
      newgamestate->hash ^= hash_castle_bk;
    }
    if (CANCASTLE_BQ(g_gamestate)) {
      newgamestate->hash ^= hash_castle_bq;
    }
    break;
  case MOVE_TYPE_CASTLEWK:
    removepiece(WHITE, KING, sqr2bbrd(W_KINGSQR));
    putpiece(WHITE, KING, sqr2bbrd(CASTLE_WK_KINGSQR));
    removepiece(WHITE, ROOK, sqr2bbrd(WK_ROOKSQR));
    putpiece(WHITE, ROOK, sqr2bbrd(CASTLE_WK_ROOKSQR));

    newgamestate->hash ^= hash_piecesqr[player][W_KINGSQR][KING];
    newgamestate->hash ^= hash_piecesqr[player][CASTLE_WK_KINGSQR][KING];
    newgamestate->hash ^= hash_piecesqr[player][WK_ROOKSQR][ROOK];
    newgamestate->hash ^= hash_piecesqr[player][CASTLE_WK_ROOKSQR][ROOK];

    newgamestate->flags &= ~(GAME_F_CANCASTLE_WK | GAME_F_CANCASTLE_WQ);
    if (CANCASTLE_WK(g_gamestate)) {
      newgamestate->hash ^= hash_castle_wk;
    }
    if (CANCASTLE_WQ(g_gamestate)) {
      newgamestate->hash ^= hash_castle_wq;
    }
    break;
  case MOVE_TYPE_CASTLEBK:
    removepiece(BLACK, KING, sqr2bbrd(B_KINGSQR));
    putpiece(BLACK, KING, sqr2bbrd(CASTLE_BK_KINGSQR));
    removepiece(BLACK, ROOK, sqr2bbrd(BK_ROOKSQR));
    putpiece(BLACK, ROOK, sqr2bbrd(CASTLE_BK_ROOKSQR));

    newgamestate->hash ^= hash_piecesqr[player][B_KINGSQR][KING];
    newgamestate->hash ^= hash_piecesqr[player][CASTLE_BK_KINGSQR][KING];
    newgamestate->hash ^= hash_piecesqr[player][BK_ROOKSQR][ROOK];
    newgamestate->hash ^= hash_piecesqr[player][CASTLE_BK_ROOKSQR][ROOK];

    newgamestate->flags &= ~(GAME_F_CANCASTLE_BK | GAME_F_CANCASTLE_BQ);
    if (CANCASTLE_BK(g_gamestate)) {
      newgamestate->hash ^= hash_castle_bk;
    }
    if (CANCASTLE_BQ(g_gamestate)) {
      newgamestate->hash ^= hash_castle_bq;
    }
    break;
  case MOVE_TYPE_NORMALPROM:
    removepiece(player, PAWN, srcbbrd);
    putpiece(player, GET_PROM_PIECE(move), dstbbrd);

    newgamestate->halfmove = 0;
    newgamestate->hash ^= hash_piecesqr[player][srcsqr][PAWN];
    newgamestate->hash ^= hash_piecesqr[player][dstsqr][GET_PROM_PIECE(move)];
    break;
  case MOVE_TYPE_NORMALCAPT:
    removepiece(!player, GET_CAPT_PIECE(move), dstbbrd);
    removepiece(player, GET_MOV_PIECE(move), srcbbrd);
    putpiece(player, GET_MOV_PIECE(move), dstbbrd);

    newgamestate->hash ^= hash_piecesqr[!player][dstsqr][GET_CAPT_PIECE(move)];
    newgamestate->hash ^= hash_piecesqr[player][srcsqr][GET_MOV_PIECE(move)];
    newgamestate->hash ^= hash_piecesqr[player][dstsqr][GET_MOV_PIECE(move)];
    if (GET_CAPT_PIECE(move) == ROOK) {
      switch (GET_DST_SQR(move)) {
      case WK_ROOKSQR:
        newgamestate->flags &= ~GAME_F_CANCASTLE_WK;
        if (CANCASTLE_WK(g_gamestate)) {
          newgamestate->hash ^= hash_castle_wk;
        }
        break;
      case BK_ROOKSQR:
        newgamestate->flags &= ~GAME_F_CANCASTLE_BK;
        if (CANCASTLE_BK(g_gamestate)) {
          newgamestate->hash ^= hash_castle_bk;
        }
        break;
      case WQ_ROOKSQR:
        newgamestate->flags &= ~GAME_F_CANCASTLE_WQ;
        if (CANCASTLE_WQ(g_gamestate)) {
          newgamestate->hash ^= hash_castle_wq;
        }
        break;
      case BQ_ROOKSQR:
        newgamestate->flags &= ~GAME_F_CANCASTLE_BQ;
        if (CANCASTLE_BQ(g_gamestate)) {
          newgamestate->hash ^= hash_castle_bq;
        }
        break;
      default:
        break;
      }
    }
    newgamestate->halfmove = 0;
    break;
  case MOVE_TYPE_PROMCAPT:
    removepiece(!player, GET_CAPT_PIECE(move), dstbbrd);
    removepiece(player, GET_MOV_PIECE(move), srcbbrd);
    putpiece(player, GET_PROM_PIECE(move), dstbbrd);

    newgamestate->hash ^= hash_piecesqr[player][srcsqr][PAWN];
    newgamestate->hash ^= hash_piecesqr[player][dstsqr][GET_PROM_PIECE(move)];
    newgamestate->hash ^= hash_piecesqr[!player][dstsqr][GET_CAPT_PIECE(move)];
    if (GET_CAPT_PIECE(move) == ROOK) {
      switch (GET_DST_SQR(move)) {
      case WK_ROOKSQR:
        newgamestate->flags &= ~GAME_F_CANCASTLE_WK;
        if (CANCASTLE_WK(g_gamestate)) {
          newgamestate->hash ^= hash_castle_wk;
        }
        break;
      case BK_ROOKSQR:
        newgamestate->flags &= ~GAME_F_CANCASTLE_BK;
        if (CANCASTLE_BK(g_gamestate)) {
          newgamestate->hash ^= hash_castle_bk;
        }
        break;
      case WQ_ROOKSQR:
        newgamestate->flags &= ~GAME_F_CANCASTLE_WQ;
        if (CANCASTLE_WQ(g_gamestate)) {
          newgamestate->hash ^= hash_castle_wq;
        }
        break;
      case BQ_ROOKSQR:
        newgamestate->flags &= ~GAME_F_CANCASTLE_BQ;
        if (CANCASTLE_BQ(g_gamestate)) {
          newgamestate->hash ^= hash_castle_bq;
        }
        break;
      default:
        break;
      }
    }
    newgamestate->halfmove = 0;
    break;
  case MOVE_TYPE_EP:
    if (player == WHITE) {
      removepiece(!player, PAWN, g_gamestate->epbbrd >> 8);
      newgamestate->hash ^=
          hash_piecesqr[!player][bbrd2sqr(g_gamestate->epbbrd >> 8)][PAWN];
    } else {
      removepiece(!player, PAWN, g_gamestate->epbbrd << 8);
      newgamestate->hash ^=
          hash_piecesqr[!player][bbrd2sqr(g_gamestate->epbbrd << 8)][PAWN];
    }
    removepiece(player, PAWN, srcbbrd);
    putpiece(player, PAWN, dstbbrd);

    newgamestate->hash ^= hash_piecesqr[player][srcsqr][PAWN];
    newgamestate->hash ^= hash_piecesqr[player][dstsqr][PAWN];
    newgamestate->halfmove = 0;
    break;
  case MOVE_TYPE_DOUBLEPUSH:
    removepiece(player, PAWN, srcbbrd);
    putpiece(player, PAWN, dstbbrd);

    newgamestate->hash ^= hash_piecesqr[player][srcsqr][PAWN];
    newgamestate->hash ^= hash_piecesqr[player][dstsqr][PAWN];
    newgamestate->hash ^= hash_epfile[srcsqr % 8];
    if (player == WHITE)
      newgamestate->epbbrd = srcbbrd << 8;
    else
      newgamestate->epbbrd = srcbbrd >> 8;
    newgamestate->halfmove = 0;
    break;
  case MOVE_TYPE_NORMAL:
    removepiece(player, GET_MOV_PIECE(move), srcbbrd);
    putpiece(player, GET_MOV_PIECE(move), dstbbrd);

    newgamestate->hash ^= hash_piecesqr[player][srcsqr][GET_MOV_PIECE(move)];
    newgamestate->hash ^= hash_piecesqr[player][dstsqr][GET_MOV_PIECE(move)];
    if (GET_MOV_PIECE(move) == PAWN)
      newgamestate->halfmove = 0;
    break;
  case MOVE_TYPE_NULL:
    break;
  default:
#ifdef DEBUG_INTERFACE
    PRINTDBG("ERROR Incorrect move type: 0x%lx\n", GET_TYPE(move));
#endif
    break;
  }

  if (popcnt(g_game.piecesof[ANY]) <= 6) {
    newgamestate->phase = PHASE_ENDGAME;
  } else if (newgamestate->fullmove <= 10) {
    newgamestate->phase = PHASE_OPENING;
  } else {
    newgamestate->phase = PHASE_MIDDLEGAME;
  }

  g_game.movelist.move[g_game.movelist.cnt] = move;
  g_game.movelist.cnt++;
  g_game.who2move = !g_game.who2move;

  update_gamestate();
}

void unmakemove() {
  g_game.who2move = !g_game.who2move;
  g_game.movelist.cnt--;

  Move move = g_game.movelist.move[g_game.movelist.cnt];

  const int player = g_game.who2move;
  const BitBrd srcbbrd = sqr2bbrd(GET_SRC_SQR(move));
  const BitBrd dstbbrd = sqr2bbrd(GET_DST_SQR(move));
  const GameState *prevgamestate = g_gamestate - 1;

  switch (GET_TYPE(move)) {
  case MOVE_TYPE_CASTLEWQ:
    removepiece(WHITE, KING, sqr2bbrd(CASTLE_WQ_KINGSQR));
    putpiece(WHITE, KING, sqr2bbrd(W_KINGSQR));
    removepiece(WHITE, ROOK, sqr2bbrd(CASTLE_WQ_ROOKSQR));
    putpiece(WHITE, ROOK, sqr2bbrd(WQ_ROOKSQR));
    break;
  case MOVE_TYPE_CASTLEBQ:
    removepiece(BLACK, KING, sqr2bbrd(CASTLE_BQ_KINGSQR));
    putpiece(BLACK, KING, sqr2bbrd(B_KINGSQR));
    removepiece(BLACK, ROOK, sqr2bbrd(CASTLE_BQ_ROOKSQR));
    putpiece(BLACK, ROOK, sqr2bbrd(BQ_ROOKSQR));
    break;
  case MOVE_TYPE_CASTLEWK:
    removepiece(WHITE, KING, sqr2bbrd(CASTLE_WK_KINGSQR));
    putpiece(WHITE, KING, sqr2bbrd(W_KINGSQR));
    removepiece(WHITE, ROOK, sqr2bbrd(CASTLE_WK_ROOKSQR));
    putpiece(WHITE, ROOK, sqr2bbrd(WK_ROOKSQR));
    break;
  case MOVE_TYPE_CASTLEBK:
    removepiece(BLACK, KING, sqr2bbrd(CASTLE_BK_KINGSQR));
    putpiece(BLACK, KING, sqr2bbrd(B_KINGSQR));
    removepiece(BLACK, ROOK, sqr2bbrd(CASTLE_BK_ROOKSQR));
    putpiece(BLACK, ROOK, sqr2bbrd(BK_ROOKSQR));
    break;
  case MOVE_TYPE_NORMALPROM:
    removepiece(player, GET_PROM_PIECE(move), dstbbrd);
    putpiece(player, PAWN, srcbbrd);
    break;
  case MOVE_TYPE_NORMALCAPT:
    removepiece(player, GET_MOV_PIECE(move), dstbbrd);
    putpiece(player, GET_MOV_PIECE(move), srcbbrd);
    putpiece(!player, GET_CAPT_PIECE(move), dstbbrd);
    break;
  case MOVE_TYPE_PROMCAPT:
    removepiece(player, GET_PROM_PIECE(move), dstbbrd);
    putpiece(player, PAWN, srcbbrd);
    putpiece(!player, GET_CAPT_PIECE(move), dstbbrd);
    break;
  case MOVE_TYPE_EP:
    if (player == WHITE) {
      putpiece(!player, PAWN, prevgamestate->epbbrd >> 8);
    } else {
      putpiece(!player, PAWN, prevgamestate->epbbrd << 8);
    }
    removepiece(player, PAWN, dstbbrd);
    putpiece(player, PAWN, srcbbrd);
    break;
  case MOVE_TYPE_DOUBLEPUSH:
    removepiece(player, PAWN, dstbbrd);
    putpiece(player, PAWN, srcbbrd);
    break;
  case MOVE_TYPE_NORMAL:
    removepiece(player, GET_MOV_PIECE(move), dstbbrd);
    putpiece(player, GET_MOV_PIECE(move), srcbbrd);
    break;
  case MOVE_TYPE_NULL:
    break;
  default:
    break;
  }

  update_gamestate();
}
