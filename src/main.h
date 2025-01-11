#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// =============================
//         Interface
// =============================

#define MODE_CLIARG 0
#define MODE_UCI 1
#define MODE_DEBUG 2

extern int g_mode;

extern int g_ucidebug;
void uci_start();

#ifdef DEBUG_INTERFACE
void debug_start();
#endif

typedef struct {
  int disbl_nmp;
  int disbl_tt;
} Settings;

extern Settings g_set;

// =============================
//     Board representation
// =============================

typedef uint64_t BitBrd;

// =============================
//      Pieces definitions
// =============================

#define PAWN 0
#define KING 1
#define KNIGHT 2
#define BISHOP 3
#define ROOK 4
#define QUEEN 5

#define PIECE_MAX 6

#define PIECE_NONE -1

// =============================
//        Move definitions
// =============================

typedef uint32_t Move;

// |31   21|20       18|17       15|14      12|11     6|5      0|
// | Flags | PromPiece | CaptPiece | MovPiece | DstSqr | SrcSqr |
// | 11bit |   3bit    |   3bit    |   3bit   |  6bit  |  6bit  |

#define MOVE_SRC_SQR_MASK 0b00000000000000000000000000111111UL
#define MOVE_DST_SQR_MASK 0b00000000000000000000111111000000UL
#define MOVE_MOV_PIECE_MASK 0b00000000000000000111000000000000UL
#define MOVE_CAPT_PIECE_MASK 0b00000000000000111000000000000000UL
#define MOVE_PROM_PIECE_MASK 0b00000000000111000000000000000000UL
#define MOVE_TYPE_MASK 0b00111111111000000000000000000000UL
#define MOVE_TYPE_NULL 0b00000000000000000000000000000000UL
#define MOVE_TYPE_CASTLEWQ 0b00000000001000000000000000000000UL
#define MOVE_TYPE_CASTLEWK 0b00000000010000000000000000000000UL
#define MOVE_TYPE_CASTLEBQ 0b00000000011000000000000000000000UL
#define MOVE_TYPE_CASTLEBK 0b00000000100000000000000000000000UL
#define MOVE_TYPE_NORMALPROM 0b00000000101000000000000000000000UL
#define MOVE_TYPE_NORMALCAPT 0b00000000110000000000000000000000UL
#define MOVE_TYPE_PROMCAPT 0b00000000111000000000000000000000UL
#define MOVE_TYPE_EP 0b00000001000000000000000000000000UL
#define MOVE_TYPE_DOUBLEPUSH 0b00000001001000000000000000000000UL
#define MOVE_TYPE_NORMAL 0b00000001010000000000000000000000UL

#define NULLMOVE 0

#define GET_SRC_SQR(m) ((int)((m) & MOVE_SRC_SQR_MASK))
#define GET_DST_SQR(m) (((int)((m) & MOVE_DST_SQR_MASK) >> 6))
#define GET_MOV_PIECE(m) (((int)((m) & MOVE_MOV_PIECE_MASK) >> 12))
#define GET_CAPT_PIECE(m) (((int)((m) & MOVE_CAPT_PIECE_MASK) >> 15))
#define GET_PROM_PIECE(m) (((int)((m) & MOVE_PROM_PIECE_MASK) >> 18))
#define GET_TYPE(m) ((m) & MOVE_TYPE_MASK)
#define GET_CASLE_FLAGS(m) ((m) & MOVE_CASTLE_FLAGS_MASK)

#define SRC_SQR(s) ((Move)s)
#define DST_SQR(s) (((Move)(s)) << 6)
#define MOV_PIECE(p) (((Move)(p)) << 12)
#define CAPT_PIECE(p) (((Move)(p)) << 15)
#define PROM_PIECE(p) (((Move)(p)) << 18)

#define IS_PROM(m)                                                             \
  (GET_TYPE(m) == MOVE_TYPE_NORMALPROM || GET_TYPE(m) == MOVE_TYPE_PROMCAPT)
#define IS_CAPT(m)                                                             \
  (GET_TYPE(m) == MOVE_TYPE_NORMALCAPT || GET_TYPE(m) == MOVE_TYPE_PROMCAPT)

#define IS_CASTLE(m)                                                           \
  (GET_TYPE(m) >= MOVE_TYPE_CASTLEWQ && GET_TYPE(m) <= MOVE_TYPE_CASTLEBK)

#define IS_SILENT(m) (!IS_CAPT(m) && !IS_PROM(m))

// =============================
//        Search definitions
// =============================

#define MAX_PLY_PER_GAME 512

typedef struct {
  Move move[MAX_PLY_PER_GAME];
  int cnt;
} MoveList;

static inline void popmove(MoveList *moves) { moves->cnt--; }

static inline void pushmove(MoveList *moves, Move m) {
  moves->move[moves->cnt] = m;
  moves->cnt++;
}

static inline void pushprommove(MoveList *moves, Move m) {
  for (int prompiece = KNIGHT; prompiece <= QUEEN; prompiece++) {
    moves->move[moves->cnt] = m | PROM_PIECE(prompiece);
    moves->cnt++;
  }
}

// =============================
//        Game definitions
// =============================

#define WHITE 0
#define BLACK 1
#define ANY 2

#define GAME_F_CANCASTLE_WK 1
#define GAME_F_CANCASTLE_WQ 2
#define GAME_F_CANCASTLE_BK 4
#define GAME_F_CANCASTLE_BQ 8

#define GAME_F_DEFAULT                                                         \
  (GAME_F_CANCASTLE_WK | GAME_F_CANCASTLE_WQ | GAME_F_CANCASTLE_BK |           \
   GAME_F_CANCASTLE_BQ)

#define CANCASTLE_WK(s) ((s)->flags & GAME_F_CANCASTLE_WK)
#define CANCASTLE_WQ(s) ((s)->flags & GAME_F_CANCASTLE_WQ)
#define CANCASTLE_BK(s) ((s)->flags & GAME_F_CANCASTLE_BK)
#define CANCASTLE_BQ(s) ((s)->flags & GAME_F_CANCASTLE_BQ)

#define FEN_STR_MAX 59

typedef struct {
  int halfmove;
  int fullmove;
  uint8_t flags;
  BitBrd epbbrd;
  BitBrd hash;
} GameState;

typedef struct {
  int who2move;

  BitBrd pieces[3][PIECE_MAX];
  BitBrd piecesof[3];

  MoveList movelist;
  GameState brdstate[MAX_PLY_PER_GAME];
} Game;

extern Game g_game;
extern GameState *g_gamestate;

int getpieceat(int color, BitBrd bbrd);
void reset_game();
char *parsefen(char *fen);
void makemove(Move move);
void unmakemove();
void move2str(char *buff, Move move);
Move parsemove(const char *str);
int getrepetitions();

// =============================
//   Move generator definitions
// =============================

void genmoves(int player, MoveList *movelist, BitBrd *attackbbrd);
void genquiet(int player, MoveList *movelist, BitBrd *attackbbrd);
void gencapt(int player, MoveList *movelist, BitBrd *attackbbrd);
int sqr_attackedby(int attacker, int sqr);

// =============================
//        Bitboard bits
// =============================

#define W_KING_SQR 4
#define B_KING_SQR 60

#define RANK_1 0xffULL
#define RANK_2 0xff00ULL
#define RANK_3 0xff0000ULL
#define RANK_4 0xff000000ULL
#define RANK_5 0xff00000000ULL
#define RANK_6 0xff0000000000ULL
#define RANK_7 0xff000000000000ULL
#define RANK_8 0xff00000000000000ULL

#define FILE_A 0x101010101010101ULL
#define FILE_B 0x202020202020202ULL
#define FILE_C 0x404040404040404ULL
#define FILE_D 0x808080808080808ULL
#define FILE_E 0x1010101010101010ULL
#define FILE_F 0x2020202020202020ULL
#define FILE_G 0x4040404040404040ULL
#define FILE_H 0x8080808080808080ULL

// rank - file + 7
#define DIAG_0 0x80ULL
#define DIAG_1 0x8040ULL
#define DIAG_2 0x804020ULL
#define DIAG_3 0x80402010ULL
#define DIAG_4 0x8040201008ULL
#define DIAG_5 0x804020100804ULL
#define DIAG_6 0x80402010080402ULL
#define DIAG_7 0x8040201008040201ULL
#define DIAG_8 0x4020100804020100ULL
#define DIAG_9 0x2010080402010000ULL
#define DIAG_10 0x1008040201000000ULL
#define DIAG_11 0x804020100000000ULL
#define DIAG_12 0x402010000000000ULL
#define DIAG_13 0x201000000000000ULL
#define DIAG_14 0x100000000000000ULL

// rank + file
#define ADIAG_0 0x1ULL
#define ADIAG_1 0x102ULL
#define ADIAG_2 0x10204ULL
#define ADIAG_3 0x1020408ULL
#define ADIAG_4 0x102040810ULL
#define ADIAG_5 0x10204081020ULL
#define ADIAG_6 0x1020408102040ULL
#define ADIAG_7 0x102040810204080ULL
#define ADIAG_8 0x204081020408000ULL
#define ADIAG_9 0x408102040800000ULL
#define ADIAG_10 0x810204080000000ULL
#define ADIAG_11 0x1020408000000000ULL
#define ADIAG_12 0x2040800000000000ULL
#define ADIAG_13 0x4080000000000000ULL
#define ADIAG_14 0x8000000000000000ULL

#define CENTER ((FILE_D | FILE_E) & (RANK_4 | RANK_5))

#define W_KINGSQR 4
#define B_KINGSQR 60

#define WK_ROOKSQR 7
#define BK_ROOKSQR 63
#define WQ_ROOKSQR 0
#define BQ_ROOKSQR 56

#define CASTLE_WK_KINGSQR 6
#define CASTLE_WK_ROOKSQR 5
#define CASTLE_BK_KINGSQR 62
#define CASTLE_BK_ROOKSQR 61
#define CASTLE_WQ_KINGSQR 2
#define CASTLE_WQ_ROOKSQR 3
#define CASTLE_BQ_KINGSQR 58
#define CASTLE_BQ_ROOKSQR 59

// =============================
//            Precomp
// =============================

typedef struct {
  BitBrd knightmask[64];
  BitBrd kingmask[64];
  BitBrd bishoppremask[64];
  BitBrd bishoppostmask[64];
  BitBrd rookpremask[64];
  BitBrd rookpostmask[64];
  BitBrd queenpremask[64];
  BitBrd queenpostmask[64];
  BitBrd pawnattackmask[2][64];

  int rookmagicshift[64];
  int bishopmagicshift[64];
  BitBrd rookmagic[64];
  BitBrd bishopmagic[64];

  // Serialized dynamically - n is not constant
  // BitBrd rookmagicmoves[64][n];
  // BitBrd bishopmagicmoves[64][n];

} PrecompTableSerialized;

typedef struct {
  BitBrd knightmask[64];
  BitBrd kingmask[64];
  BitBrd bishoppremask[64];
  BitBrd bishoppostmask[64];
  BitBrd rookpremask[64];
  BitBrd rookpostmask[64];
  BitBrd queenpremask[64];
  BitBrd queenpostmask[64];
  BitBrd pawnattackmask[2][64];

  int rookmagicshift[64];
  int bishopmagicshift[64];
  BitBrd rookmagic[64];
  BitBrd bishopmagic[64];

  BitBrd *rookmagicmoves[64];
  BitBrd *bishopmagicmoves[64];

} PrecompTable;

extern PrecompTable g_precomp;

void genprecomp();
void huntmagic();
void usemagic(const char *numstr);
int loadprecomp();
void freeprecomp();

// =============================
//          Zobrist hash
// =============================

void inithash();
BitBrd gethash();

// =============================
//          Evaluation
// =============================

static const int pawnval = 100;
static const int knightval = 300;
static const int bishopval = 310;
static const int rookval = 500;
static const int queenval = 800;

static const int material[] = {pawnval,   0,       knightval,
                               bishopval, rookval, queenval};

#define SCORE_CHECKMATE 99999999
#define SCORE_CHECKMATE_BOUND (SCORE_CHECKMATE - 256)

#define SCORE_CHECKMATED (-SCORE_CHECKMATE)
#define SCORE_ILLEGAL (SCORE_CHECKMATED - 1)

#define IS_CHECKMATE(score)                                                    \
  (((score) >= SCORE_CHECKMATE_BOUND && (score) <= SCORE_CHECKMATE) ||         \
   ((score) <= -SCORE_CHECKMATE_BOUND && (score) >= -SCORE_CHECKMATE))

int evaluate(int pliescnt);

// =============================
//             Search
// =============================

#define DEPTH_INF -1

#define STOP_MANUAL 0
#define STOP_TIME 1

void reset_hashtables();
void search(int requesteddepth);
void stop(int origin);

// =============================
//             Dump
// =============================

void dumpprecomp();
void dumppos();

// =============================
//           Perft
// =============================

void perft(int depth, int *nodes, int *leafnodes);

// =============================
//           Utils
// =============================

static inline int bbrd2sqr(BitBrd bbrd) { return __builtin_ffsll(bbrd) - 1; }

static inline BitBrd sqr2bbrd(int sqr) { return (1ULL << (BitBrd)sqr); }

static inline int sqr2diag(int sqr) { return (sqr / 8) - (sqr % 8) + 7; }

static inline int sqr2antidiag(int sqr) { return (sqr / 8) + (sqr % 8); }

static inline int equals(const char *s1, const char *s2) {
  return strcmp(s1, s2) == 0;
}

static inline int popcnt(BitBrd bbrd) {
  int cnt = 0;

  while (bbrd) {
    cnt++;
    bbrd &= ~sqr2bbrd(bbrd2sqr(bbrd));
  }

  return cnt;
}

static inline BitBrd file2rank(BitBrd bbrd) { return (bbrd * DIAG_7) >> 56; }

static inline BitBrd rank2file(BitBrd bbrd) {
  return (bbrd * ADIAG_7) & FILE_A;
}

static inline BitBrd nextsubset(BitBrd subset, BitBrd set) {
  return (subset - set) & set;
}

static inline char *nexttok() { return strtok(NULL, " \n"); }
static inline char *nexttok_untilend() { return strtok(NULL, "\n"); }

static inline int lastmovelegal() {
  return !sqr_attackedby(g_game.who2move,
                         bbrd2sqr(g_game.pieces[!g_game.who2move][KING]));
}
static inline int undercheck() {
  return sqr_attackedby(!g_game.who2move,
                        bbrd2sqr(g_game.pieces[g_game.who2move][KING]));
}

static inline int checking() {
  return sqr_attackedby(g_game.who2move,
                        bbrd2sqr(g_game.pieces[!g_game.who2move][KING]));
}

static inline BitBrd rand64() { return rand() | (((BitBrd)rand()) << 32); }

#endif
