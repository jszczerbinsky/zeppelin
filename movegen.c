#include "main.h"

static void gen_sliding(int player, MoveList* movelist, int piece)
{
	BitBrd piecesbbrd = g_game.pieces[player][piece];

	while (piecesbbrd)
	{
		int    piecesqr = bbrd2sqr(piecesbbrd);
		BitBrd dstbbrd	= 0ULL;

		if (piece == ROOK || piece == QUEEN)
		{
			BitBrd occ = g_game.piecesof[ANY] & g_precomp.rookpremask[piecesqr];
			BitBrd index = (occ * g_precomp.rookmagic[piecesqr]) >>
				g_precomp.rookmagicshift[piecesqr];
			dstbbrd |= g_precomp.rookmagicmoves[piecesqr][index];
		}
		if (piece == BISHOP || piece == QUEEN)
		{
			BitBrd occ =
				g_game.piecesof[ANY] & g_precomp.bishoppremask[piecesqr];
			BitBrd index = (occ * g_precomp.bishopmagic[piecesqr]) >>
				g_precomp.bishopmagicshift[piecesqr];
			dstbbrd |= g_precomp.bishopmagicmoves[piecesqr][index];
		}

		dstbbrd &= ~g_game.piecesof[player];

		while (dstbbrd)
		{
			int dstsqr = bbrd2sqr(dstbbrd);

			Move move = SRC_SQR(piecesqr) | DST_SQR(dstsqr) | MOV_PIECE(piece);

			if (dstbbrd & g_game.piecesof[!player])
				move |=
					MOVE_F_ISCAPT | CAPT_PIECE(getpieceat(!player, dstbbrd));

			pushmove(movelist, move);

			dstbbrd &= ~sqr2bbrd(dstsqr);
		}

		piecesbbrd &= ~sqr2bbrd(piecesqr);
	}
}

static void gen_knight(int player, MoveList* movelist)
{
	BitBrd knightsbbrd = g_game.pieces[player][KNIGHT];

	while (knightsbbrd)
	{
		int knightsqr = bbrd2sqr(knightsbbrd);

		BitBrd dstbbrd =
			g_precomp.knightmask[knightsqr] & (~g_game.piecesof[player]);

		while (dstbbrd)
		{
			int dstsqr = bbrd2sqr(dstbbrd);

			Move move =
				SRC_SQR(knightsqr) | DST_SQR(dstsqr) | MOV_PIECE(KNIGHT);

			if (dstbbrd & g_game.piecesof[!player])
				move |=
					MOVE_F_ISCAPT | CAPT_PIECE(getpieceat(!player, dstbbrd));

			pushmove(movelist, move);

			dstbbrd &= ~sqr2bbrd(dstsqr);
		}

		knightsbbrd &= ~sqr2bbrd(knightsqr);
	}
}

static void gen_king(int player, MoveList* movelist)
{
	int kingsqr = bbrd2sqr(g_game.pieces[player][KING]);

	BitBrd dstbbrd = g_precomp.kingmask[kingsqr] & (~g_game.piecesof[player]);

	while (dstbbrd)
	{
		int dstsqr = bbrd2sqr(dstbbrd);

		Move move = SRC_SQR(kingsqr) | DST_SQR(dstsqr) | MOV_PIECE(KING);

		if (dstbbrd & g_game.piecesof[!player])
			move |= MOVE_F_ISCAPT | CAPT_PIECE(getpieceat(!player, dstbbrd));

		pushmove(movelist, move);

		dstbbrd &= ~sqr2bbrd(dstsqr);
	}
}

static void gen_castle(int player, MoveList* movelist)
{
	if (player == WHITE)
	{
		if (CANCASTLE_WK(g_gamestate)) pushmove(movelist, MOVE_F_ISCASTLEWK);
		if (CANCASTLE_WQ(g_gamestate)) pushmove(movelist, MOVE_F_ISCASTLEWQ);
	}
	else
	{
		if (CANCASTLE_BK(g_gamestate)) pushmove(movelist, MOVE_F_ISCASTLEBK);
		if (CANCASTLE_BQ(g_gamestate)) pushmove(movelist, MOVE_F_ISCASTLEBQ);
	}
}

static void gen_pawncapt(int player, MoveList* movelist)
{
	const BitBrd player_pawns = g_game.pieces[player][PAWN];

	BitBrd l_dstbbrd, r_dstbbrd;

	if (player == WHITE)
	{
		l_dstbbrd = ((player_pawns & (~FILE_A)) << 7) &
			(g_game.piecesof[!player] | g_gamestate->epbbrd);
		r_dstbbrd = ((player_pawns & (~FILE_H)) << 9) &
			(g_game.piecesof[!player] | g_gamestate->epbbrd);
	}
	else
	{
		l_dstbbrd = ((player_pawns & (~FILE_H)) >> 7) &
			(g_game.piecesof[!player] | g_gamestate->epbbrd);
		r_dstbbrd = ((player_pawns & (~FILE_A)) >> 9) &
			(g_game.piecesof[!player] | g_gamestate->epbbrd);
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

static void gen_pushprom(int player, MoveList* movelist)
{
	const BitBrd player_pawns = g_game.pieces[player][PAWN];

	BitBrd dstbbrd;

	if (player == WHITE)
		dstbbrd = (player_pawns << 8) & (~g_game.piecesof[ANY]) & RANK_8;
	else
		dstbbrd = (player_pawns >> 8) & (~g_game.piecesof[ANY]) & RANK_1;

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

static void gen_pawnsilent(int player, MoveList* movelist)
{
	BitBrd single_dstbbrd, double_dstbbrd;

	if (player == WHITE)
	{
		single_dstbbrd = (g_game.pieces[player][PAWN] << 8) &
			(~g_game.piecesof[ANY]) & (~RANK_8);
		double_dstbbrd =
			((single_dstbbrd & RANK_3) << 8) & (~g_game.piecesof[ANY]);
	}
	else
	{
		single_dstbbrd = (g_game.pieces[player][PAWN] >> 8) &
			(~g_game.piecesof[ANY]) & (~RANK_1);
		double_dstbbrd =
			((single_dstbbrd & RANK_6) >> 8) & (~g_game.piecesof[ANY]);
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

void genmoves(int player, MoveList* movelist)
{
	movelist->cnt = 0;
	gen_pawnsilent(player, movelist);
	gen_pushprom(player, movelist);
	gen_pawncapt(player, movelist);
	gen_king(player, movelist);
	gen_castle(player, movelist);
	gen_knight(player, movelist);
	gen_sliding(player, movelist, ROOK);
	gen_sliding(player, movelist, BISHOP);
	gen_sliding(player, movelist, QUEEN);
}
