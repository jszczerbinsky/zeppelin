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

static void dumpprecomp_magic(
		FILE* f, BitBrd magic[64], int shift[64], BitBrd* magicmoves[64],
		BitBrd premask[64]
		)
{
	for (int sqr = 0; sqr < 64; sqr++)
	{
		fprintf(f, "Square: %d\n", sqr);
		fprintf(f, "Magic number: 0x%lx\n", magic[sqr]);
		fprintf(f, "Shitft (64-indexsize): %d\n", shift[sqr]);
		fprintf(f, "Index size: %d\n", 64 - shift[sqr]);

		char strocc[64][2];
		char strmov[64][2];

		BitBrd subset = 0;

		do
		{
			subset = nextsubset(subset, premask[sqr]);

			int index = (subset * magic[sqr]) >> shift[sqr];

			for (int i = 0; i < 64; i++)
			{
				if (i == sqr)
				{
					strocc[i][0] = 'o';
					strocc[i][1] = ' ';
					strmov[i][0] = 'o';
					strmov[i][1] = ' ';
				}
				else
				{
					strocc[i][0] = (subset & sqr2bbrd(i)) ? 'x' : ' ';
					strocc[i][1] = ' ';
					strmov[i][0] =
						(magicmoves[sqr][index] & sqr2bbrd(i)) ? 'x' : ' ';
					strmov[i][1] = ' ';
				}
			}
			fprintf(f, "Occupation board:\n");
			printbrd(f, strocc);
			fprintf(f, "Possible moves:\n");
			printbrd(f, strmov);
			fprintf(f, "\n");

		} while (subset);
	}
}

void dumpprecomp()
{
	FILE* f = init_dumpfile("dump-precomp.txt");

	printsection(f, "PRECOMPUTED KNIGHT MOVES");
	dumpprecomp_moves(f, g_precomp.knightmask);

	printsection(f, "PRECOMPUTED KING MOVES");
	dumpprecomp_moves(f, g_precomp.kingmask);

	printsection(f, "PRECOMPUTED BISHOP MOVES PREMASK");
	dumpprecomp_moves(f, g_precomp.bishoppremask);

	printsection(f, "PRECOMPUTED BISHOP MOVES POSTMASK");
	dumpprecomp_moves(f, g_precomp.bishoppostmask);

	printsection(f, "PRECOMPUTED ROOK MOVES PREMASK");
	dumpprecomp_moves(f, g_precomp.rookpremask);

	printsection(f, "PRECOMPUTED ROOK MOVES POSTMASK");
	dumpprecomp_moves(f, g_precomp.rookpostmask);

	printsection(f, "PRECOMPUTED QUEEN MOVES PREMASK");
	dumpprecomp_moves(f, g_precomp.queenpremask);

	printsection(f, "PRECOMPUTED QUEEN MOVES POSTMASK");
	dumpprecomp_moves(f, g_precomp.queenpostmask);

	printsection(f, "ROOK MAGIC");
	dumpprecomp_magic(
			f,
			g_precomp.rookmagic,
			g_precomp.rookmagicshift,
			g_precomp.rookmagicmoves,
			g_precomp.rookpremask
			);

	printsection(f, "BISHOP MAGIC");
	dumpprecomp_magic(
			f,
			g_precomp.bishopmagic,
			g_precomp.bishopmagicshift,
			g_precomp.bishopmagicmoves,
			g_precomp.bishoppremask
			);

	printsection(f, "WHITE PAWN ATTACKS");
	dumpprecomp_moves(f, g_precomp.pawnattackmask[WHITE]);

	printsection(f, "BLACK PAWN ATTACKS");
	dumpprecomp_moves(f, g_precomp.pawnattackmask[BLACK]);

	fclose(f);
}

void dumppos()
{
	FILE* f = init_dumpfile("dump-position.txt");

	fprintf(f, "%s on move\n\n", g_game.who2move == WHITE ? "White" : "Black");

	char str[64][2];
	for (int i = 0; i < 64; i++)
	{
		int piece = getpieceat(ANY, sqr2bbrd(i));
		int color = (sqr2bbrd(i) & g_game.piecesof[WHITE]) ? WHITE : BLACK;
		str[i][0] = piece2char(color, piece);
		str[i][1] = color == BLACK ? ' ' : '.';
	}
	printbrd(f, str);

	MoveList movelist;
	genmoves(g_game.who2move, &movelist);

	fprintf(f, "\nAvailable moves:\n");
	for (int i = 0; i < movelist.cnt; i++)
	{
		char buff[6];
		move2str(buff, movelist.move[i]);
		fprintf(f, "%s\n", buff);
	}

	fclose(f);
}
