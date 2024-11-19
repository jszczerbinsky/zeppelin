#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

static void sendboard()
{
	printf("{\n");

	printf("\"wpawn\": %lu,\n", g_game.pieces[WHITE][PAWN]);
	printf("\"wking\": %lu,\n", g_game.pieces[WHITE][KING]);
	printf("\"wknight\": %lu,\n", g_game.pieces[WHITE][KNIGHT]);
	printf("\"wbishop\": %lu,\n", g_game.pieces[WHITE][BISHOP]);
	printf("\"wrook\": %lu,\n", g_game.pieces[WHITE][ROOK]);
	printf("\"wqueen\": %lu,\n", g_game.pieces[WHITE][QUEEN]);
	printf("\"bpawn\": %lu,\n", g_game.pieces[BLACK][PAWN]);
	printf("\"bking\": %lu,\n", g_game.pieces[BLACK][KING]);
	printf("\"bknight\": %lu,\n", g_game.pieces[BLACK][KNIGHT]);
	printf("\"bbishop\": %lu,\n", g_game.pieces[BLACK][BISHOP]);
	printf("\"brook\": %lu,\n", g_game.pieces[BLACK][ROOK]);
	printf("\"bqueen\": %lu,\n", g_game.pieces[BLACK][QUEEN]);

	printf("\"player\": \"%s\",\n", g_game.who2move == WHITE ? "w" : "b");

	printf("\"wk\": %s,\n", CANCASTLE_WK(g_gamestate) ? "true" : "false");
	printf("\"wq\": %s,\n", CANCASTLE_WQ(g_gamestate) ? "true" : "false");
	printf("\"bk\": %s,\n", CANCASTLE_BK(g_gamestate) ? "true" : "false");
	printf("\"bq\": %s,\n", CANCASTLE_BQ(g_gamestate) ? "true" : "false");

	printf("\"ep\": %lu,\n", g_gamestate->epbbrd);

	printf("\"halfmove\": %u,\n", g_gamestate->halfmove);
	printf("\"fullmove\": %u\n", g_gamestate->fullmove);

	printf("}\n");
}

static void finishsending()
{
	printf("END\n");
	fflush(stdout);
}

static void respond2fencheck(char* fen)
{
	parsefen(fen);
	sendboard();
	finishsending();
}

static void respond2unmakemovecheck(char* fenbak)
{
	char fen[1024];
	strcpy(fen, fenbak);

	parsefen(fen);
	strcpy(fen, fenbak);

	MoveList movelist;
	genmoves(g_game.who2move, &movelist);

	printf("{ \"moves\": [\n");
	for (int i = 0; i < movelist.cnt; i++)
	{
		parsefen(fen);
		strcpy(fen, fenbak);

		char buff[6];
		move2str(buff, movelist.move[i]);

		printf("{\n");
		printf("\"move\": \"%s\",\n", buff);
		printf("\"before\": ");
		sendboard();

		makemove(movelist.move[i]);
		unmakemove();

		printf(",\n \"after\": ");
		sendboard();

		if (i == movelist.cnt - 1)
			printf("}\n");
		else
			printf("},\n");
	}
	printf("]}\n");
	finishsending();
}

static int next_cmd(char* buff, int len)
{
	char* token = strtok(buff, " \n");
	if (!token) return 0;

	if (equals(token, "fencheck"))
		respond2fencheck(nexttok_untilend());
	else if (equals(token, "unmakemovecheck"))
		respond2unmakemovecheck(nexttok_untilend());
	else if (equals(token, "quit"))
		return 1;

	return 0;
}

void debug_start()
{
	g_mode = MODE_DEBUG;

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
