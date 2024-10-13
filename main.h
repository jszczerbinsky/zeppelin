#ifndef MAIN_H
#define MAIN_H

#include <stdint.h>

// =============================
//         Interface
// =============================

int uci_start();

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

// =============================
//        Move definitions
// =============================

typedef uint32_t Move;

// |31   21|20       18|17       15|14      12|11     6|5      0|
// | Flags | PromPiece | CaptPiece | MovPiece | DstSqr | SrcSqr |
// | 11bit |   3bit    |   3bit    |   3bit   |  6bit  |  6bit  |

#define MOVE_SRC_SQR_MASK 		0b00000000000000000000000000111111
#define MOVE_DST_SQR_MASK 		0b00000000000000000000111111000000
#define MOVE_MOV_PIECE_MASK 	0b00000000000000000111000000000000
#define MOVE_CAPT_PIECE_MASK 	0b00000000000000111000000000000000
#define MOVE_PROM_PIECE_MASK 	0b00000000000111000000000000000000
#define MOVE_F_ISPROM			0b00000000001000000000000000000000
#define MOVE_F_ISCAPT			0b00000000010000000000000000000000
#define MOVE_F_ISEP				0b00000000100000000000000000000000
#define MOVE_F_ISCASTLEQ		0b00000001000000000000000000000000
#define MOVE_F_ISCASTLEK		0b00000010000000000000000000000000

#define GET_SRC_SQR(m) 		((m) & MOVE_SRC_SQR_MASK)
#define GET_DST_SQR(m) 		(((m) & MOVE_SRC_SQR_MASK)>>6)
#define GET_MOV_PIECE(m) 	(((m) & MOVE_PIECE_MASK)>>12)
#define GET_CAPT_PIECE(m) 	(((m) & MOVE_CAPT_PIECE_MASK)>>15)
#define GET_PROM_PIECE(m) 	(((m) & MOVE_PROM_PIECE_MASK)>>18)

#define SRC_SQR(s) 		(s)
#define DST_SQR(s) 		((s)<<6)
#define MOV_PIECE(p) 	((p)<<12)
#define CAPT_PIECE(p) 	((p)<<15)
#define PROM_PIECE(p) 	((p)<<18)

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
	Move ply[MAX_PLY_PER_GAME];
	int  plycnt;
} MoveList;

// =============================
//        Game definitions
// =============================

#define WHITE 0
#define BLACK 1

#define GAME_F_CANCASTLE_WK 1
#define GAME_F_CANCASTLE_WQ 2
#define GAME_F_CANCASTLE_BK 4
#define GAME_F_CANCASTLE_BQ 8
#define GAME_F_EPPOSSIBLE	16

#define GAME_F_DEFAULT (GAME_F_CANCASTLE_WK | GAME_F_CANCASTLE_WQ | GAME_F_CANCASTLE_BK | GAME_F_CANCASTLE_BQ)

typedef struct
{
	int halfmove;
	int fullmove;
	uint8_t flags;
	BitBrd epbrd;
} BoardState;

typedef struct
{
	int who2move;

	BitBrd pieces[2][PIECE_MAX];
	BitBrd playerpieces[2];
	BitBrd allpieces;

	MoveList movelist;
	BoardState brdstate[MAX_PLY_PER_GAME];
} Game;

void update_bbrds(Game* game);
void reset_game(Game* game);
void parse_fen(Game* game, char* fen);
void makemove(Game* game, Move move);
void undomove(Game* game);

#endif
