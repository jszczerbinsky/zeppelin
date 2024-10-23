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

void precomp()
{
	gen_kingmoves();

	FILE* out = fopen("precomputed.bin", "wb");
	fwrite(&g_precomp, sizeof(PrecompTable), 1, out);
	fclose(out);
}
