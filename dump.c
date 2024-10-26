#include <stdio.h>

#include "main.h"

static char piece2char(int color, int piece)
{
	switch (piece)
	{
		case PAWN:
			return color == WHITE ? 'P' : 'p';
		case KING:
			return color == WHITE ? 'K' : 'k';
		case KNIGHT:
			return color == WHITE ? 'N' : 'n';
		case BISHOP:
			return color == WHITE ? 'B' : 'b';
		case ROOK:
			return color == WHITE ? 'R' : 'r';
		case QUEEN:
			return color == WHITE ? 'Q' : 'q';
		default:
			return ' ';
	}
}

static void printbrd(FILE* f, const char str[64][2])
{
	for (int rank = 7; rank >= 0; rank--)
	{
		if (rank == 7)
			fprintf(f, " ┌──┬──┬──┬──┬──┬──┬──┬──┐\n");
		else
			fprintf(f, " ├──┼──┼──┼──┼──┼──┼──┼──┤\n");

		for (int file = 0; file < 8; file++)
		{
			if (file == 0) fprintf(f, "%d", rank + 1);

			int sqr = file + rank * 8;
			fprintf(f, "│%c%c", str[sqr][0], str[sqr][1]);

			if (file == 7) fprintf(f, "│");
		}
		fprintf(f, "\n");
		if (rank == 0)
		{
			fprintf(f, " └──┴──┴──┴──┴──┴──┴──┴──┘\n");
			fprintf(f, "  A  B  C  D  E  F  G  H  \n");
		}
	}
}

static void printsection(FILE* f, const char* name)
{
	fprintf(f, "==============================\n");

	int namelen = strlen(name);
	int leftpad = 30 / 2 - namelen / 2;

	for (int i = 0; i < leftpad; i++) fputc(' ', f);

	fprintf(f, "%s\n", name);
	fprintf(f, "==============================\n");
}

static FILE* init_dumpfile(const char* name) { return fopen(name, "w"); }

static void dumpprecomp_moves(FILE* f, BitBrd bbrd[64])
{
	for (int sqr = 0; sqr < 64; sqr++)
	{
		fprintf(f, "Square: %d\n", sqr);
		char str[64][2];
		for (int i = 0; i < 64; i++)
		{
			if (i == sqr)
			{
				str[i][0] = 'o';
				str[i][1] = ' ';
			}
			else
			{
				str[i][0] = (bbrd[sqr] & sqr2bbrd(i)) ? 'x' : ' ';
				str[i][1] = ' ';
			}
		}
		printbrd(f, str);
		fprintf(f, "\n");
	}
}

void dumpprecomp()
{
	FILE* f = init_dumpfile("dump-precomp.txt");

	printsection(f, "PRECOMPUTED KNIGHT MOVES");
	dumpprecomp_moves(f, g_precomp.knightmoves);

	printsection(f, "PRECOMPUTED KING MOVES");
	dumpprecomp_moves(f, g_precomp.kingmoves);

	printsection(f, "PRECOMPUTED BISHOP MOVES");
	dumpprecomp_moves(f, g_precomp.bishopmoves);

	printsection(f, "PRECOMPUTED ROOK MOVES");
	dumpprecomp_moves(f, g_precomp.rookmoves);

	printsection(f, "PRECOMPUTED QUEEN MOVES");
	dumpprecomp_moves(f, g_precomp.queenmoves);

	fclose(f);
}
