#include <stdio.h>
#include <stdlib.h>

#include "main.h"

#define GEN_NORMAL 0
#define GEN_HUNT   1

PrecompTable g_precomp;

static int file_loaded;

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

static void savefile()
{
	FILE* out = fopen("precomputed.bin", "wb");

	fwrite(&g_precomp.knightmask, sizeof(BitBrd), 64, out);
	fwrite(&g_precomp.kingmask, sizeof(BitBrd), 64, out);
	fwrite(&g_precomp.bishoppremask, sizeof(BitBrd), 64, out);
	fwrite(&g_precomp.bishoppostmask, sizeof(BitBrd), 64, out);
	fwrite(&g_precomp.rookpremask, sizeof(BitBrd), 64, out);
	fwrite(&g_precomp.rookpostmask, sizeof(BitBrd), 64, out);
	fwrite(&g_precomp.queenpremask, sizeof(BitBrd), 64, out);
	fwrite(&g_precomp.queenpostmask, sizeof(BitBrd), 64, out);

	fwrite(&g_precomp.rookmagicshift, sizeof(int), 64, out);
	fwrite(&g_precomp.bishopmagicshift, sizeof(int), 64, out);
	fwrite(&g_precomp.rookmagic, sizeof(BitBrd), 64, out);
	fwrite(&g_precomp.bishopmagic, sizeof(BitBrd), 64, out);

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

static int ismagic_checklen(int sqr, int indexlen, int piece, BitBrd num)
{
	BitBrd premask = piece == ROOK ? g_precomp.rookpremask[sqr]
		: g_precomp.bishoppremask[sqr];

	BitBrd usedby[1 << indexlen];
	int	   used[1 << indexlen];
	memset(usedby, 0, (1 << indexlen) * sizeof(BitBrd));
	memset(used, 0, (1 << indexlen) * sizeof(int));

	BitBrd subset = 0;
	do
	{
		subset	     = nextsubset(subset, premask);
		BitBrd index = ((subset * num) >> (64 - indexlen));

		BitBrd magicmoves = gen_magicmoves(sqr, subset, piece);

		if (used[index] != 0 && usedby[index] != magicmoves) return 0;

		usedby[index] = magicmoves;
		used[index]   = 1;

	} while (subset);

	return 1;
}

int ismagic(int sqr, int piece, BitBrd num)
{
	BitBrd premask = piece == ROOK ? g_precomp.rookpremask[sqr]
		: g_precomp.bishoppremask[sqr];

	int indexlen = popcnt(premask);

	if (!ismagic_checklen(sqr, indexlen, piece, num)) return 0;

	while (indexlen > 0)
	{
		if (!ismagic_checklen(sqr, indexlen - 1, piece, num)) return indexlen;
		indexlen--;
	}

	return 0;
}

static void update_magicmoves_for(int piece, int sqr)
{
	int shift = piece == ROOK ? g_precomp.rookmagicshift[sqr]
		: g_precomp.bishopmagicshift[sqr];

	BitBrd premask = piece == ROOK ? g_precomp.rookpremask[sqr]
		: g_precomp.bishoppremask[sqr];

	int arrsize = 1 << (64 - shift);

	if (piece == ROOK)
	{
		if (file_loaded) free(g_precomp.rookmagicmoves[sqr]);
		g_precomp.rookmagicmoves[sqr] = malloc(arrsize * sizeof(BitBrd));
	}
	else
	{
		if (file_loaded) free(g_precomp.bishopmagicmoves[sqr]);
		g_precomp.bishopmagicmoves[sqr] = malloc(arrsize * sizeof(BitBrd));
	}

	BitBrd subset = 0;
	do
	{
		subset = nextsubset(subset, premask);

		if (piece == ROOK)
		{
			BitBrd index = (subset * g_precomp.rookmagic[sqr]) >>
				g_precomp.rookmagicshift[sqr];
			g_precomp.rookmagicmoves[sqr][index] =
				gen_magicmoves(sqr, subset, ROOK);
		}
		else
		{
			BitBrd index = (subset * g_precomp.bishopmagic[sqr]) >>
				g_precomp.bishopmagicshift[sqr];

			g_precomp.bishopmagicmoves[sqr][index] =
				gen_magicmoves(sqr, subset, BISHOP);
		}
	} while (subset);
}

static void gen_magic_for(int piece, int sqr, int mode)
{
	BitBrd num;
	int	   indexlen;

	int tries	     = 100000000;
	int previndexlen = 64 - (piece == ROOK ? g_precomp.rookmagicshift[sqr]
			: g_precomp.bishopmagicshift[sqr]);

	do
	{
		do
		{
			num = rand() | (((BitBrd)rand()) << 32);
			num &= rand() | (((BitBrd)rand()) << 32);

			indexlen = ismagic(sqr, piece, num);

			tries--;
			if (mode == GEN_HUNT && tries <= 0) return;

		} while (indexlen == 0);

	} while (mode == GEN_HUNT && indexlen >= previndexlen);

	if (piece == ROOK)
	{
		g_precomp.rookmagicshift[sqr] = 64 - indexlen;
		g_precomp.rookmagic[sqr]      = num;
	}
	else
	{
		g_precomp.bishopmagicshift[sqr] = 64 - indexlen;
		g_precomp.bishopmagic[sqr]	= num;
	}

	update_magicmoves_for(piece, sqr);

	if (mode == GEN_NORMAL)
		printf(
				"Found %s magic number for square %d: 0x%lx (index len: "
				"%d, array size: %d)\n",
				piece == BISHOP ? "bishop" : "rook",
				sqr,
				num,
				indexlen,
				1 << indexlen
			  );
	else
	{
		printf("Hunted a better magic number!\n");
		printf("Number: 0x%lx\n", num);
		printf("Index length: %d -> %d\n", previndexlen, indexlen);
		printf(
				"Decreased the size of precomp file by %d bytes\n\n",
				(previndexlen - indexlen) * 4
			  );
		savefile();
	}
}

static void gen_magic()
{
	if (file_loaded)
	{
		printf(
				"There are magic numbers in the precomp file, if they work, they "
				"will NOT be overriden.\n"
			  );

		for (int i = 0; i < 64; i++)
		{
			if (ismagic(i, ROOK, g_precomp.rookmagic[i]) == 0)
			{
				printf(
						"Rook magic number for square %d is incorrect, "
						"regenerating\n",
						i
					  );
				gen_magic_for(ROOK, i, GEN_NORMAL);
			}
			if (ismagic(i, BISHOP, g_precomp.bishopmagic[i]) == 0)
			{
				printf(
						"Bishop magic number for square %d is incorrect, "
						"regenerating\n",
						i
					  );
				gen_magic_for(BISHOP, i, GEN_NORMAL);
			}
		}

		printf("Magic numbers checking done\n");
	}
	else
	{
		for (int i = 0; i < 64; i++)
		{
			gen_magic_for(ROOK, i, GEN_NORMAL);
			gen_magic_for(BISHOP, i, GEN_NORMAL);
		}
	}
}

void usemagic(const char* numstr)
{
	file_loaded = 1;

	BitBrd num = strtoll(numstr, NULL, 16);

	if (num == 0)
	{
		fprintf(stderr, "Specified number format is incorrect\n");
		return;
	}

	printf("Checking if specified number is magic for any square\n");

	int foundany = 0;
	for (int sqr = 0; sqr < 64; sqr++)
	{
		int rook_indexlen   = ismagic(sqr, ROOK, num);
		int bishop_indexlen = ismagic(sqr, BISHOP, num);

		if (rook_indexlen != 0)
		{
			printf(
					"Specified number is magic for rook on square %d with index "
					"size %d\n",
					sqr,
					rook_indexlen
				  );
			if (rook_indexlen >= (64 - g_precomp.rookmagicshift[sqr]))
			{
				printf(
						"We already use a magic number with same/better index "
						"size\n"
					  );
			}
			else
			{
				g_precomp.rookmagic[sqr]      = num;
				g_precomp.rookmagicshift[sqr] = 64 - rook_indexlen;
				update_magicmoves_for(ROOK, sqr);
				foundany = 1;
			}
		}

		if (bishop_indexlen != 0)
		{
			printf(
					"Specified number is magic for bishop on square %d with index "
					"size %d\n",
					sqr,
					bishop_indexlen
				  );
			if (bishop_indexlen >= (64 - g_precomp.bishopmagicshift[sqr]))
			{
				printf(
						"We already use a magic number with same/better index "
						"size\n"
					  );
			}
			else
			{
				g_precomp.bishopmagic[sqr]	= num;
				g_precomp.bishopmagicshift[sqr] = 64 - rook_indexlen;
				update_magicmoves_for(BISHOP, sqr);
				foundany = 1;
			}
		}
	}

	if (foundany)
	{
		printf("Magic numbers updated\n");
		savefile();
	}
	else
		printf("No magic numbers have been updated\n");
}

void huntmagic()
{
	while (1)
	{
		int piece = rand() % 2 ? ROOK : BISHOP;
		int sqr	  = rand() % 64;
		printf(
				"Trying to hunt a better magic number for %s on square %d\n",
				piece == ROOK ? "rook" : "bishop",
				sqr
			  );

		gen_magic_for(piece, sqr, GEN_HUNT);
	}
}

int loadprecomp()
{
	FILE* in = fopen("precomputed.bin", "rb");
	if (!in) return 0;

	fread(&g_precomp.knightmask, sizeof(BitBrd), 64, in);
	fread(&g_precomp.kingmask, sizeof(BitBrd), 64, in);
	fread(&g_precomp.bishoppremask, sizeof(BitBrd), 64, in);
	fread(&g_precomp.bishoppostmask, sizeof(BitBrd), 64, in);
	fread(&g_precomp.rookpremask, sizeof(BitBrd), 64, in);
	fread(&g_precomp.rookpostmask, sizeof(BitBrd), 64, in);
	fread(&g_precomp.queenpremask, sizeof(BitBrd), 64, in);
	fread(&g_precomp.queenpostmask, sizeof(BitBrd), 64, in);

	fread(&g_precomp.rookmagicshift, sizeof(int), 64, in);
	fread(&g_precomp.bishopmagicshift, sizeof(int), 64, in);
	fread(&g_precomp.rookmagic, sizeof(BitBrd), 64, in);
	fread(&g_precomp.bishopmagic, sizeof(BitBrd), 64, in);

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
	file_loaded = loadprecomp();

	gen_kingmask();
	gen_knightmask();
	gen_slidingmoves();
	gen_magic();

	savefile();
}

void freeprecomp()
{
	for (int i = 0; i < 64; i++)
	{
		free(g_precomp.rookmagicmoves[i]);
		free(g_precomp.bishopmagicmoves[i]);
	}
}
