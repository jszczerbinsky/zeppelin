#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

// =============================
//         Interface
// =============================

#define MODE_CLI 0
#define MODE_UCI 1

extern int g_mode;

int equals(const char* s1, const char* s2);

void cli_start();

extern int g_ucidebug;
void uci_start();

// =============================
//     Board representation
// =============================

typedef uint64_t BitBrd;

int 	bbrd2sqr(BitBrd bbrd);
BitBrd 	sqr2bbrd(int sqr);
int 	popcnt(BitBrd bbrd);

// =============================
//      Pieces definitions
// =============================

#define PAWN 	0
#define KING 	1
#define KNIGHT 	2
#define BISHOP 	3
#define ROOK 	4
#define QUEEN 	5

#define PIECE_MAX 6

#define PIECE_NONE -1

// =============================
//        Move definitions
// =============================

typedef uint32_t Move;

// |31   21|20       18|17       15|14      12|11     6|5      0|
// | Flags | PromPiece | CaptPiece | MovPiece | DstSqr | SrcSqr |
// | 11bit |   3bit    |   3bit    |   3bit   |  6bit  |  6bit  |

#define MOVE_SRC_SQR_MASK 		0b00000000000000000000000000111111U
#define MOVE_DST_SQR_MASK 		0b00000000000000000000111111000000U
#define MOVE_MOV_PIECE_MASK 	0b00000000000000000111000000000000U
#define MOVE_CAPT_PIECE_MASK 	0b00000000000000111000000000000000U
#define MOVE_PROM_PIECE_MASK 	0b00000000000111000000000000000000U
#define MOVE_F_ISPROM			0b00000000001000000000000000000000U
#define MOVE_F_ISCAPT			0b00000000010000000000000000000000U
#define MOVE_F_ISEP				0b00000000100000000000000000000000U
#define MOVE_F_ISCASTLEQ		0b00000001000000000000000000000000U
#define MOVE_F_ISCASTLEK		0b00000010000000000000000000000000U
#define MOVE_F_ISDOUBLEPUSH		0b00000100000000000000000000000000U

#define NULLMOVE 0

#define GET_SRC_SQR(m) 		((int)((m) & MOVE_SRC_SQR_MASK))
#define GET_DST_SQR(m) 		(((int)((m) & MOVE_DST_SQR_MASK)>>6))
#define GET_MOV_PIECE(m) 	(((int)((m) & MOVE_MOV_PIECE_MASK)>>12))
#define GET_CAPT_PIECE(m) 	(((int)((m) & MOVE_CAPT_PIECE_MASK)>>15))
#define GET_PROM_PIECE(m) 	(((int)((m) & MOVE_PROM_PIECE_MASK)>>18))

#define SRC_SQR(s) 		((Move)s)
#define DST_SQR(s) 		(((Move)(s))<<6)
#define MOV_PIECE(p) 	(((Move)(p))<<12)
#define CAPT_PIECE(p) 	(((Move)(p))<<15)
#define PROM_PIECE(p) 	(((Move)(p))<<18)

#define IS_PROM(m) 		((m) & MOVE_F_ISPROM)
#define IS_CAPT(m) 		((m) & MOVE_F_ISCAPT)
#define IS_EP(m)		((m) & MOVE_F_ISEP)
#define IS_CASTLEQ(m)	((m) & MOVE_F_ISCASTLEQ)
#define IS_CASTLEK(m)	((m) & MOVE_F_ISCASTLEK)
#define IS_CASTLE(m)	(IS_CASTLEK(m) || IS_CASTLEQ(m))

// =============================
//        Search definitions
// =============================

#define MAX_PLY_PER_GAME 512

typedef struct
{
	Move move[MAX_PLY_PER_GAME];
	int  cnt;
} MoveList;

void pushmove(MoveList* movelist, Move move);
void pushprommove(MoveList* movelist, Move move);

// =============================
//        Game definitions
// =============================

#define WHITE 	0
#define BLACK 	1
#define ANY		2

#define GAME_F_CANCASTLE_WK 1
#define GAME_F_CANCASTLE_WQ 2
#define GAME_F_CANCASTLE_BK 4
#define GAME_F_CANCASTLE_BQ 8

#define GAME_F_DEFAULT (GAME_F_CANCASTLE_WK | GAME_F_CANCASTLE_WQ | GAME_F_CANCASTLE_BK | GAME_F_CANCASTLE_BQ)

#define CANCASTLE_WK(s) ((s)->flags & GAME_F_CANCASTLE_WK)
#define CANCASTLE_WQ(s) ((s)->flags & GAME_F_CANCASTLE_WQ)
#define CANCASTLE_BK(s) ((s)->flags & GAME_F_CANCASTLE_BK)
#define CANCASTLE_BQ(s) ((s)->flags & GAME_F_CANCASTLE_BQ)

#define FEN_STR_MAX 59

typedef struct
{
	int halfmove;
	int fullmove;
	uint8_t flags;
	BitBrd epbbrd;
} GameState;

typedef struct
{
	int who2move;

	BitBrd pieces[3][PIECE_MAX];
	BitBrd piecesof[3];

	MoveList movelist;
	GameState brdstate[MAX_PLY_PER_GAME];
} Game;

extern Game g_game;
extern GameState* g_gamestate;

int getpieceat(int color, BitBrd bbrd);
void update_bbrds();
void reset_game();
int parsefen(char* fen);
void makemove(Move move);
void undomove();
void getfen(char* buff);

// =============================
//   Move generator definitions
// =============================

void genmoves(const Game* game, int player, MoveList* movelist);

// =============================
//        Bitboard bits
// =============================

#define RANK_1 0xffULL
#define RANK_2 0xff00ULL
#define RANK_3 0xff0000ULL
#define RANK_4 0xff000000ULL
#define RANK_5 0xff00000000ULL
#define RANK_6 0xff0000000000ULL
#define RANK_7 0xff000000000000ULL
#define RANK_8 0xff00000000000000ULL

#define FILE_A 0x101010101010101ULL
#define FILE_H 0x8080808080808080ULL

// =============================
//            Precomp
// =============================

typedef struct
{
	BitBrd knightmoves[64];
	BitBrd kingmoves[64];
} PrecompTable;

extern PrecompTable g_precomp;

void precomp();

// =============================
//             Logs
// =============================

#ifdef LOG_ENABLE
	void initlog();
	void printlog(const char* format, ...);
	#define LOG_INIT() initlog()
	#define LOG(...) printlog(__VA_ARGS__)
#else // #ifdef LOG_ENABLE
	#define LOG_INIT()
	#define LOG(...)
#endif // #ifdef LOG_ENABLE

#endif // #ifdef MAIN_H
