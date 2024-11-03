#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#define TOK_DELIMS " \n"

int g_ucidebug = 0;

static inline char* nexttok() { return strtok(NULL, TOK_DELIMS); }
static inline char* nexttok_untilend() { return strtok(NULL, "\0"); }

static void respond2uci()
{
	printf("id name testengine\n");
	printf("id author Jakub Szczerbinski\n");
	printf("uciok\n");
	fflush(stdout);
}

static Move parsemove(const char* str)
{
	int srcsqr = (str[0] - 'a') + (str[1] - '1') * 8;
	int dstsqr = (str[2] - 'a') + (str[3] - '1') * 8;

	BitBrd srcbbrd = sqr2bbrd(srcsqr);
	BitBrd dstbbrd = sqr2bbrd(dstsqr);

	const int enemy    = !g_game.who2move;
	const int movpiece = getpieceat(g_game.who2move, srcbbrd);

	if (movpiece == KING)
	{
		if (dstsqr == 2 && g_game.who2move == WHITE &&
				CANCASTLE_WQ(g_gamestate))
			return MOVE_F_ISCASTLEWQ;
		if (dstsqr == 6 && g_game.who2move == WHITE &&
				CANCASTLE_WK(g_gamestate))
			return MOVE_F_ISCASTLEWK;
		if (dstsqr == 58 && g_game.who2move == BLACK &&
				CANCASTLE_BQ(g_gamestate))
			return MOVE_F_ISCASTLEBQ;
		if (dstsqr == 62 && g_game.who2move == BLACK &&
				CANCASTLE_BK(g_gamestate))
			return MOVE_F_ISCASTLEBK;
	}

	Move move = SRC_SQR(srcsqr) | DST_SQR(dstsqr) | MOV_PIECE(movpiece);

	if (g_game.piecesof[enemy] & dstbbrd)
	{
		int piece = getpieceat(enemy, dstbbrd);
		move |= MOVE_F_ISCAPT | CAPT_PIECE(piece);
	}
	else if (movpiece == PAWN && g_gamestate->epbbrd & dstbbrd)
	{
		move |= MOVE_F_ISEP | CAPT_PIECE(PAWN);
	}

	switch (str[4])
	{
		case 'q':
			move |= PROM_PIECE(QUEEN) | MOVE_F_ISPROM;
			break;
		case 'b':
			move |= PROM_PIECE(BISHOP) | MOVE_F_ISPROM;
			break;
		case 'r':
			move |= PROM_PIECE(ROOK) | MOVE_F_ISPROM;
			break;
		case 'n':
			move |= PROM_PIECE(KNIGHT) | MOVE_F_ISPROM;
			break;
		default:
			break;
	}

	return move;
}

static void respond2ucinewgame() {}

static void respond2position(char* token)
{
	if (equals(token, "startpos"))
		reset_game();
	else if (equals(token, "fen"))
	{
		token = nexttok_untilend();
		parsefen(token);
	}
	else
		printf("%s\n", token);

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

static void respond2debug(char* token)
{
	if (equals(token, "on"))
		g_ucidebug = 1;
	else if (equals(token, "off"))
		g_ucidebug = 0;
}

static void respond2isready()
{
	printf("readyok\n");
	fflush(stdout);
}

static int next_cmd(char* buff, int len)
{
	char* token = strtok(buff, " \n");

	if (equals(token, "uci"))
		respond2uci();
	else if (equals(token, "ucinewgame"))
		respond2ucinewgame();
	else if (equals(token, "position"))
		respond2position(nexttok());
	else if (equals(token, "debug"))
		respond2debug(nexttok());
	else if (equals(token, "isready"))
		respond2isready();
	else if (equals(token, "quit"))
		return 1;

	return 0;
}

void uci_start()
{
	g_mode = MODE_UCI;
	respond2uci();

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
