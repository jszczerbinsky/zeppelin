#include <stdio.h>
#include <stdlib.h>

#include "main.h"

PrecompTable g_precomp;

static const BitBrd rankbbrd[] = {
	RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8
};

static const BitBrd filebbrd[] = {
	FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H
};

static const BitBrd diagbbrds[] = {
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

static const BitBrd antidiagbbrds[] = {
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

static void gen_kingmask()
{
	for (int sqr = 0; sqr < 64; sqr++)
	{
		g_precomp.kingmask[sqr] = 0;

		int rank = sqr / 8;
		int file = sqr % 8;

		int n = rank < 7;
		int s = rank > 0;
		int e = file < 7;
		int w = file > 0;

		if (n) g_precomp.kingmask[sqr] |= sqr2bbrd(sqr + 8);
		if (s) g_precomp.kingmask[sqr] |= sqr2bbrd(sqr - 8);
		if (e) g_precomp.kingmask[sqr] |= sqr2bbrd(sqr + 1);
		if (w) g_precomp.kingmask[sqr] |= sqr2bbrd(sqr - 1);
		if (n && e) g_precomp.kingmask[sqr] |= sqr2bbrd(sqr + 9);
		if (n && w) g_precomp.kingmask[sqr] |= sqr2bbrd(sqr + 7);
		if (s && e) g_precomp.kingmask[sqr] |= sqr2bbrd(sqr - 7);
		if (s && w) g_precomp.kingmask[sqr] |= sqr2bbrd(sqr - 9);
	}
}

static void gen_knightmask()
{
	for (int sqr = 0; sqr < 64; sqr++)
	{
		g_precomp.knightmask[sqr] = 0;

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
				g_precomp.knightmask[sqr] |= sqr2bbrd(dstsqr[i]);

		if (bbrd & (FILE_A | FILE_B))
			g_precomp.knightmask[sqr] &= ~(FILE_G | FILE_H);
		else if (bbrd & (FILE_G | FILE_H))
			g_precomp.knightmask[sqr] &= ~(FILE_A | FILE_B);
		if (bbrd & (RANK_1 | RANK_2))
			g_precomp.knightmask[sqr] &= ~(RANK_7 | RANK_8);
		else if (bbrd & (RANK_7 | RANK_8))
			g_precomp.knightmask[sqr] &= ~(RANK_1 | RANK_2);
	}
}

static void gen_slidingmoves()
{
	for (int sqr = 0; sqr < 64; sqr++)
	{
		int rank = sqr / 8;
		int file = sqr % 8;

		g_precomp.rookpostmask[sqr] = g_precomp.rookpremask[sqr] =
			(rankbbrd[rank] | filebbrd[file]) & ~sqr2bbrd(sqr);

		g_precomp.bishoppostmask[sqr] = g_precomp.bishoppremask[sqr] =
			(diagbbrds[sqr2diag(sqr)] | antidiagbbrds[sqr2antidiag(sqr)]) &
			~sqr2bbrd(sqr);

		if (rank != 0)
		{
			g_precomp.rookpremask[sqr] &= ~RANK_1;
			g_precomp.bishoppremask[sqr] &= ~RANK_1;
		}
		if (rank != 7)
		{
			g_precomp.rookpremask[sqr] &= ~RANK_8;
			g_precomp.bishoppremask[sqr] &= ~RANK_8;
		}
		if (file != 0)
		{
			g_precomp.rookpremask[sqr] &= ~FILE_A;
			g_precomp.bishoppremask[sqr] &= ~FILE_A;
		}
		if (file != 7)
		{
			g_precomp.rookpremask[sqr] &= ~FILE_H;
			g_precomp.bishoppremask[sqr] &= ~FILE_H;
		}

		g_precomp.queenpostmask[sqr] =
			g_precomp.rookpostmask[sqr] | g_precomp.bishoppostmask[sqr];
		g_precomp.queenpremask[sqr] =
			g_precomp.rookpremask[sqr] | g_precomp.bishoppremask[sqr];
	}
}

static BitBrd gen_magicmoves(int sqr, BitBrd occupation, int piece)
{
	BitBrd result = 0;

	if (piece == ROOK)
	{
		int s	 = sqr;
		int rank = s / 8;
		while (rank < 7)
		{
			s += 8;
			rank	= s / 8;
			BitBrd bbrd = sqr2bbrd(s);
			result |= bbrd;
			if (bbrd & occupation) break;
		}
		s    = sqr;
		rank = s / 8;
		while (rank > 0)
		{
			s -= 8;
			rank	= s / 8;
			BitBrd bbrd = sqr2bbrd(s);
			result |= bbrd;
			if (bbrd & occupation) break;
		}
		s	 = sqr;
		int file = s % 8;
		while (file < 7)
		{
			s += 1;
			file	= s % 8;
			BitBrd bbrd = sqr2bbrd(s);
			result |= bbrd;
			if (bbrd & occupation) break;
		}
		s    = sqr;
		file = s % 8;
		while (file > 0)
		{
			s -= 1;
			file	= s % 8;
			BitBrd bbrd = sqr2bbrd(s);
			result |= bbrd;
			if (bbrd & occupation) break;
		}
	}
	else
	{
		int s	 = sqr;
		int rank = s / 8;
		int file = s % 8;
		while (rank < 7 && file < 7)
		{
			s += 9;
			rank	= s / 8;
			BitBrd bbrd = sqr2bbrd(s);
			result |= bbrd;
			if (bbrd & occupation) break;
		}
		s    = sqr;
		rank = s / 8;
		file = s % 8;
		while (rank > 0 && file < 7)
		{
			s -= 7;
			rank	= s / 8;
			BitBrd bbrd = sqr2bbrd(s);
			result |= bbrd;
			if (bbrd & occupation) break;
		}
		s    = sqr;
		rank = s / 8;
		file = s % 8;
		while (rank > 0 && file > 0)
		{
			s -= 9;
			file	= s % 8;
			BitBrd bbrd = sqr2bbrd(s);
			result |= bbrd;
			if (bbrd & occupation) break;
		}
		s    = sqr;
		rank = s / 8;
		file = s % 8;

		while (rank < 7 && file > 0)
		{
			s += 7;
			file	= s % 8;
			BitBrd bbrd = sqr2bbrd(s);
			result |= bbrd;
			if (bbrd & occupation) break;
		}
	}

	return result;
}

static void gen_magic_for(int piece, BitBrd premasklist[])
{
	for (int sqr = 0; sqr < 64; sqr++)
	{
		BitBrd premask = premasklist[sqr];

		int    indexlen	 = popcnt(premask);
		BitBrd indexmask = (1ULL << indexlen) - 1;

		BitBrd num;
		int    ismagic;

		BitBrd occupations[indexmask + 1];

		do
		{
			ismagic = 1;
			num	    = rand() | (((BitBrd)rand()) << 32);
			num &= rand() | (((BitBrd)rand()) << 32);

			int used[indexmask + 1];
			memset(used, 0, (indexmask + 1) * sizeof(int));

			BitBrd subset = 0;
			do
			{
				subset	     = nextsubset(subset, premask);
				BitBrd index = ((subset * num) >> (64 - indexlen)) & indexmask;

				if (used[index])
				{
					subset  = 0;
					ismagic = 0;
				}
				else
					occupations[index] = subset;

				used[index]++;

			} while (subset);

		} while (!ismagic);

		if (piece == ROOK)
		{
			g_precomp.rookmagicshift[sqr] = 64 - indexlen;
			g_precomp.rookmagic[sqr]	  = num;
			g_precomp.rookmagicmoves[sqr] =
				malloc((indexmask + 1) * sizeof(BitBrd));

			for (int i = 0; i < indexmask + 1; i++)
				g_precomp.rookmagicmoves[sqr][i] =
					gen_magicmoves(sqr, occupations[i], ROOK);
		}
		else
		{
			g_precomp.bishopmagicshift[sqr] = 64 - indexlen;
			g_precomp.bishopmagic[sqr]	    = num;
			g_precomp.bishopmagicmoves[sqr] =
				malloc((indexmask + 1) * sizeof(BitBrd));

			for (int i = 0; i < indexmask + 1; i++)
				g_precomp.bishopmagicmoves[sqr][i] =
					gen_magicmoves(sqr, occupations[i], BISHOP);
		}

		printf(
				"Found %s magic number for square %d: 0x%lx (index len: "
				"%d, array size: %d)\n",
				piece == BISHOP ? "bishop" : "rook",
				sqr,
				num,
				indexlen,
				1 << indexlen
			  );
	}
}

static void gen_magic()
{
	gen_magic_for(ROOK, g_precomp.rookpremask);
	gen_magic_for(BISHOP, g_precomp.bishoppremask);
}

int loadprecomp()
{
	FILE* in = fopen("precomputed.bin", "rb");
	if (!in) return 0;

	fread(&g_precomp, sizeof(PrecompTableSerialized), 1, in);

	for (int i = 0; i < 64; i++)
	{
		int cnt = 1 << (64 - g_precomp.rookmagicshift[i]);

		g_precomp.rookmagicmoves[i] = malloc(cnt * sizeof(BitBrd));

		fread(g_precomp.rookmagicmoves[i], sizeof(BitBrd), cnt, in);
	}
	for (int i = 0; i < 64; i++)
	{
		int cnt = 1 << (64 - g_precomp.bishopmagicshift[i]);

		g_precomp.bishopmagicmoves[i] = malloc(cnt * sizeof(BitBrd));

		fread(g_precomp.bishopmagicmoves[i], sizeof(BitBrd), cnt, in);
	}

	fclose(in);

	return 1;
}

void genprecomp()
{
	gen_kingmask();
	gen_knightmask();
	gen_slidingmoves();
	gen_magic();

	FILE* out = fopen("precomputed.bin", "wb");
	fwrite(&g_precomp, sizeof(PrecompTableSerialized), 1, out);

	for (int i = 0; i < 64; i++)
	{
		int cnt = 1 << (64 - g_precomp.rookmagicshift[i]);
		fwrite(g_precomp.rookmagicmoves[i], sizeof(BitBrd), cnt, out);
	}
	for (int i = 0; i < 64; i++)
	{
		int cnt = 1 << (64 - g_precomp.bishopmagicshift[i]);
		fwrite(g_precomp.bishopmagicmoves[i], sizeof(BitBrd), cnt, out);
	}

	fclose(out);
}

void freeprecomp()
{
	for (int i = 0; i < 64; i++)
	{
		free(g_precomp.rookmagicmoves[i]);
		free(g_precomp.bishopmagicmoves[i]);
	}
}
