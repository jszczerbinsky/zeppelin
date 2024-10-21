#include "main.h"

inline int bbrd2sqr(BitBrd bbrd) { return __builtin_ffsll(bbrd) - 1; }

inline BitBrd sqr2bbrd(int sqr) { return (1ULL << (BitBrd)sqr); }

int popcnt(BitBrd bbrd) { return 0; }

void pushmove(MoveList* moves, Move m)
{
	moves->move[moves->cnt] = m;
	moves->cnt++;
}

void pushprommove(MoveList* moves, Move m)
{
	for (int prompiece = KNIGHT; prompiece <= QUEEN; prompiece++)
	{
		moves->move[moves->cnt] = m | PROM_PIECE(prompiece) | MOVE_F_ISPROM;
		moves->cnt++;
	}
}
