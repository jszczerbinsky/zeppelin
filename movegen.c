#include "main.h"

static void gen_pawncapt(const Game* game, int player, MoveList* movelist)
{
	const BitBrd player_pawns = game->pieces[player][PAWN];

	BitBrd l_dstbbrd, r_dstbbrd;

	if (player == WHITE)
	{
		l_dstbbrd = ((player_pawns & (~FILE_A)) << 7) &
			(game->piecesof[!player] | g_gamestate->epbbrd);
		r_dstbbrd = ((player_pawns & (~FILE_H)) << 9) &
			(game->piecesof[!player] | g_gamestate->epbbrd);
	}
	else
	{
		l_dstbbrd = ((player_pawns & (~FILE_H)) >> 7) &
			(game->piecesof[!player] | g_gamestate->epbbrd);
		r_dstbbrd = ((player_pawns & (~FILE_A)) >> 9) &
			(game->piecesof[!player] | g_gamestate->epbbrd);
	}

	while (l_dstbbrd)
	{
		int    dstsqr  = bbrd2sqr(l_dstbbrd);
		BitBrd dstbbrd = sqr2bbrd(dstsqr);
		int    srcsqr;

		if (player == WHITE)
			srcsqr = dstsqr - 7;
		else
			srcsqr = dstsqr + 7;

		Move move =
			SRC_SQR(srcsqr) | DST_SQR(dstsqr) | MOV_PIECE(PAWN) | MOVE_F_ISCAPT;

		if (dstbbrd & g_gamestate->epbbrd)
			move |= MOVE_F_ISEP | CAPT_PIECE(PAWN);
		else
			move |= CAPT_PIECE(getpieceat(!player, dstbbrd));

		const int isprom = (player == WHITE && (dstbbrd & RANK_8)) ||
			(player == BLACK && (dstbbrd & RANK_1));

		if (isprom)
			pushprommove(movelist, move);
		else
			pushmove(movelist, move);

		l_dstbbrd &= ~sqr2bbrd(dstsqr);
	}

	while (r_dstbbrd)
	{
		int    dstsqr  = bbrd2sqr(r_dstbbrd);
		BitBrd dstbbrd = sqr2bbrd(dstsqr);
		int    srcsqr;

		if (player == WHITE)
			srcsqr = dstsqr - 9;
		else
			srcsqr = dstsqr + 9;

		Move move =
			SRC_SQR(srcsqr) | DST_SQR(dstsqr) | MOV_PIECE(PAWN) | MOVE_F_ISCAPT;

		if (dstbbrd & g_gamestate->epbbrd)
			move |= MOVE_F_ISEP | CAPT_PIECE(PAWN);
		else
			move |= CAPT_PIECE(getpieceat(!player, dstbbrd));

		const int isprom = (player == WHITE && (dstbbrd & RANK_8)) ||
			(player == BLACK && (dstbbrd & RANK_1));

		if (isprom)
			pushprommove(movelist, move);
		else
			pushmove(movelist, move);

		r_dstbbrd &= ~sqr2bbrd(dstsqr);
	}
}

static void gen_pushprom(const Game* game, int player, MoveList* movelist)
{
	const BitBrd player_pawns = game->pieces[player][PAWN];

	BitBrd dstbbrd;

	if (player == WHITE)
		dstbbrd = (player_pawns << 8) & (~game->piecesof[ANY]) & RANK_8;
	else
		dstbbrd = (player_pawns >> 8) & (~game->piecesof[ANY]) & RANK_1;

	while (dstbbrd)
	{
		int dstsqr = bbrd2sqr(dstbbrd);
		int srcsqr;

		if (player == WHITE)
			srcsqr = dstsqr - 8;
		else
			srcsqr = dstsqr + 8;

		Move move = SRC_SQR(srcsqr) | DST_SQR(dstsqr) | MOV_PIECE(PAWN);
		pushprommove(movelist, move);

		dstbbrd &= ~sqr2bbrd(dstsqr);
	}
}

static void gen_pawnsilent(const Game* game, int player, MoveList* movelist)
{
	BitBrd single_dstbbrd, double_dstbbrd;

	if (player == WHITE)
	{
		single_dstbbrd = (game->pieces[player][PAWN] << 8) &
			(~game->piecesof[ANY]) & (~RANK_8);
		double_dstbbrd =
			((single_dstbbrd & RANK_3) << 8) & (~game->piecesof[ANY]);
	}
	else
	{
		single_dstbbrd = (game->pieces[player][PAWN] >> 8) &
			(~game->piecesof[ANY]) & (~RANK_1);
		double_dstbbrd =
			((single_dstbbrd & RANK_6) >> 8) & (~game->piecesof[ANY]);
	}

	while (single_dstbbrd)
	{
		int dstsqr = bbrd2sqr(single_dstbbrd);
		int srcsqr;

		if (player == WHITE)
			srcsqr = dstsqr - 8;
		else
			srcsqr = dstsqr + 8;

		Move move = SRC_SQR(srcsqr) | DST_SQR(dstsqr) | MOV_PIECE(PAWN);
		pushmove(movelist, move);

		single_dstbbrd &= ~sqr2bbrd(dstsqr);
	}

	while (double_dstbbrd)
	{
		int dstsqr = bbrd2sqr(double_dstbbrd);
		int srcsqr;

		if (player == WHITE)
			srcsqr = dstsqr - 16;
		else
			srcsqr = dstsqr + 16;

		Move move = SRC_SQR(srcsqr) | DST_SQR(dstsqr) | MOV_PIECE(PAWN) |
			MOVE_F_ISDOUBLEPUSH;
		pushmove(movelist, move);

		double_dstbbrd &= ~sqr2bbrd(dstsqr);
	}
}

void genmoves(const Game* game, int player, MoveList* movelist)
{
	gen_pawnsilent(game, player, movelist);
	gen_pushprom(game, player, movelist);
	gen_pawncapt(game, player, movelist);
}
