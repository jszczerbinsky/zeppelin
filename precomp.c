#include <stdio.h>

#include "main.h"

PrecompTable g_precomp;

static void gen_kingmoves()
{
	for (int sqr = 0; sqr < 64; sqr++)
	{
		g_precomp.kingmoves[sqr] = 0;

		int rank = sqr / 8;
		int file = sqr % 8;

		int n = rank < 7;
		int s = rank > 0;
		int e = file < 7;
		int w = file > 0;

		if (n) g_precomp.kingmoves[sqr] |= sqr2bbrd(sqr + 8);
		if (s) g_precomp.kingmoves[sqr] |= sqr2bbrd(sqr - 8);
		if (e) g_precomp.kingmoves[sqr] |= sqr2bbrd(sqr + 1);
		if (w) g_precomp.kingmoves[sqr] |= sqr2bbrd(sqr - 1);
		if (n && e) g_precomp.kingmoves[sqr] |= sqr2bbrd(sqr + 9);
		if (n && w) g_precomp.kingmoves[sqr] |= sqr2bbrd(sqr + 7);
		if (s && e) g_precomp.kingmoves[sqr] |= sqr2bbrd(sqr - 7);
		if (s && w) g_precomp.kingmoves[sqr] |= sqr2bbrd(sqr - 9);
	}
}

static void gen_knightmoves()
{
	for (int sqr = 0; sqr < 64; sqr++)
	{
		g_precomp.knightmoves[sqr] = 0;

		BitBrd bbrd = sqr2bbrd(sqr);

		int dstsqr[] = {
			sqr + 10,
			sqr + 17,
			sqr + 15,
			sqr + 6,
			sqr - 10,
			sqr - 17,
			sqr - 15,
			sqr - 6
		};

		for (int i = 0; i < 8; i++)
			if (dstsqr[i] >= 0 && dstsqr[i] < 64)
				g_precomp.knightmoves[sqr] |= sqr2bbrd(dstsqr[i]);

		if (bbrd & (FILE_A | FILE_B))
			g_precomp.knightmoves[sqr] &= ~(FILE_G | FILE_H);
		else if (bbrd & (FILE_G | FILE_H))
			g_precomp.knightmoves[sqr] &= ~(FILE_A | FILE_B);
		if (bbrd & (RANK_1 | RANK_2))
			g_precomp.knightmoves[sqr] &= ~(RANK_7 | RANK_8);
		else if (bbrd & (RANK_7 | RANK_8))
			g_precomp.knightmoves[sqr] &= ~(RANK_1 | RANK_2);
	}
}

static void gen_slidingmoves()
{
	const BitBrd rankbbrd[] = {
		RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8
	};

	const BitBrd filebbrd[] = {
		FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H
	};

	const BitBrd diagbbrds[] = {
		DIAG_0,
		DIAG_1,
		DIAG_2,
		DIAG_3,
		DIAG_4,
		DIAG_5,
		DIAG_6,
		DIAG_7,
		DIAG_8,
		DIAG_9,
		DIAG_10,
		DIAG_11,
		DIAG_12,
		DIAG_13,
		DIAG_14
	};

	const BitBrd antidiagbbrds[] = {
		ADIAG_0,
		ADIAG_1,
		ADIAG_2,
		ADIAG_3,
		ADIAG_4,
		ADIAG_5,
		ADIAG_6,
		ADIAG_7,
		ADIAG_8,
		ADIAG_9,
		ADIAG_10,
		ADIAG_11,
		ADIAG_12,
		ADIAG_13,
		ADIAG_14
	};

	for (int sqr = 0; sqr < 64; sqr++)
	{
		int rank = sqr / 8;
		int file = sqr % 8;

		g_precomp.rookmoves[sqr] = rankbbrd[rank] | filebbrd[file];

		g_precomp.bishopmoves[sqr] =
			diagbbrds[sqr2diag(sqr)] | antidiagbbrds[sqr2antidiag(sqr)];

		g_precomp.queenmoves[sqr] =
			g_precomp.rookmoves[sqr] | g_precomp.bishopmoves[sqr];
	}
}

void precomp()
{
	gen_kingmoves();
	gen_knightmoves();
	gen_slidingmoves();

	FILE* out = fopen("precomputed.bin", "wb");
	fwrite(&g_precomp, sizeof(PrecompTable), 1, out);
	fclose(out);
}
