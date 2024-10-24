#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "main.h"

#define TOK_DELIMS " \n"

static inline char* nexttok() { return strtok(NULL, TOK_DELIMS); }
static inline char* nexttok_untilend() { return strtok(NULL, "\0"); }

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

static void say(const char* str)
{
	printf("~ %s\n", str);
	fflush(stdout);
}

static void printbrd(const char str[64][2])
{
	for (int rank = 7; rank >= 0; rank--)
	{
		if (rank == 7)
			printf(" ┌──┬──┬──┬──┬──┬──┬──┬──┐\n");
		else
			printf(" ├──┼──┼──┼──┼──┼──┼──┼──┤\n");

		for (int file = 0; file < 8; file++)
		{
			if (file == 0) printf("%d", rank + 1);

			int sqr = file + rank * 8;
			printf("│%c%c", str[sqr][0], str[sqr][1]);

			if (file == 7) printf("│");
		}
		printf("\n");
		if (rank == 0)
		{
			printf(" └──┴──┴──┴──┴──┴──┴──┴──┘\n");
			printf("  A  B  C  D  E  F  G  H  \n");
		}
	}
}

static void respond2print(char* token)
{
	if (equals(token, "squares"))
	{
		char str[64][2];
		for (int i = 0; i < 64; i++)
		{
			str[i][0] = (i / 10) + '0';
			str[i][1] = (i % 10) + '0';
		}
		printbrd(str);
	}
	else if (equals(token, "precomp"))
	{
		token = nexttok();
		if (!token)
		{
			say("Incomplete command");
			return;
		}

		const BitBrd* bbrd;
		if (equals(token, "kingmoves"))
			bbrd = g_precomp.kingmoves;
		else if (equals(token, "knightmoves"))
			bbrd = g_precomp.knightmoves;
		else if (equals(token, "bishopmoves"))
			bbrd = g_precomp.bishopmoves;
		else if (equals(token, "rookmoves"))
			bbrd = g_precomp.rookmoves;
		else if (equals(token, "queenmoves"))
			bbrd = g_precomp.queenmoves;

		else
		{
			say("Unknown precomputed value");
			return;
		}

		token = nexttok();
		if (!token)
		{
			say("Incomplete command");
			return;
		}

		int sqr = atoi(token);

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
		printbrd(str);
	}
	else if (equals(token, "game"))
	{
		char str[64][2];
		for (int i = 0; i < 64; i++)
		{
			int piece = getpieceat(ANY, sqr2bbrd(i));
			int color = (sqr2bbrd(i) & g_game.piecesof[WHITE]) ? WHITE : BLACK;
			str[i][0] = piece2char(color, piece);
			str[i][1] = color == BLACK ? ' ' : '.';
		}
		printbrd(str);
	}
}

static int next_cmd(char* buff, int len)
{
	LOG("CLI: received command '%.*s'", len - 1, buff);
	char* token = strtok(buff, " \n");

	if (!token) return 0;

	if (equals(token, "uci"))
	{
		LOG("CLI: switched to UCI mode");
		uci_start();
		return 1;
	}
	else if (equals(token, "quit"))
	{
		LOG("Closing");
		return 1;
	}
	else if (equals(token, "hello"))
		say("hello");
	else if (equals(token, "genprecomp"))
	{
		precomp();
		say("done");
	}
	else if (equals(token, "print"))
		respond2print(nexttok());
	else if (equals(token, "loadfen"))
	{
		if (parsefen(nexttok_untilend()))
			say("done");
		else
			say("failed");
	}
	else if (equals(token, "getfen"))
	{
		char buff[FEN_STR_MAX];
		getfen(buff);
		printf("%s\n", buff);
	}
	else if (equals(token, "reset"))
	{
		reset_game();
		say("done");
	}
	else if (equals(token, "getmoves"))
	{
		MoveList movelist;
		genmoves(g_game.who2move, &movelist);

		printf("Moves count: %d\n", movelist.cnt);
		for (int i = 0; i < movelist.cnt; i++)
		{
			char buff[6];
			move2str(buff, movelist.move[i]);
			printf("%s\n", buff);
		}
	}

	return 0;
}

void cli_start()
{
	size_t buffsize = 256;
	char*  buff	    = malloc(buffsize * sizeof(char));

	int quit = 0;
	int len  = 0;

	while (!quit)
	{
		if ((len = getline(&buff, &buffsize, stdin)) == -1)
			quit = 1;
		else
			quit = next_cmd(buff, len);
	}

	free(buff);
}
