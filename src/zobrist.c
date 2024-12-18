#include "main.h"

static BitBrd piecekey[2][PIECE_MAX];
static BitBrd playerkey[2];
static BitBrd castekey_wk;
static BitBrd castekey_wq;
static BitBrd castekey_bk;
static BitBrd castekey_bq;
static BitBrd epfilekey[8];

void inithash()
{
	for (int player = 0; player < 3; player++)
	{
		playerkey[player] = rand64();
		for (int piece = 0; piece < PIECE_MAX; piece++)
			piecekey[player][piece] = rand64();
	}
	castekey_wk = rand64();
	castekey_wq = rand64();
	castekey_bk = rand64();
	castekey_bq = rand64();
	for (int i = 0; i < 8; i++) epfilekey[i] = rand64();
}

BitBrd gethash()
{
	BitBrd hash = 0;

	hash ^= playerkey[g_game.who2move];

	for (int sqr = 0; sqr < 64; sqr++)
	{
		BitBrd bbrd = sqr2bbrd(sqr);

		for (int player = 0; player < 2; player++)
			for (int piece = 0; piece < PIECE_MAX; piece++)
				if (g_game.pieces[player][piece] & bbrd)
					hash ^= piecekey[player][piece];

		if (g_gamestate->epbbrd & bbrd) hash ^= epfilekey[sqr % 8];
	}

	if (CANCASTLE_WK(g_gamestate)) hash ^= castekey_wk;
	if (CANCASTLE_WQ(g_gamestate)) hash ^= castekey_wq;
	if (CANCASTLE_BK(g_gamestate)) hash ^= castekey_bk;
	if (CANCASTLE_BQ(g_gamestate)) hash ^= castekey_bq;

	return hash;
}
