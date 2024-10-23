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

static void printmove(char* buff, Move move)
{
	if (IS_CASTLEK(move))
	{
		if (g_game.who2move == WHITE)
			strcpy(buff, "e1g1");
		else
			strcpy(buff, "e8g8");
	}
	else if (IS_CASTLEQ(move))
	{
		if (g_game.who2move == WHITE)
			strcpy(buff, "e1c1");
		else
			strcpy(buff, "e8c8");
	}

	int srcsqr = GET_SRC_SQR(move);
	int dstsqr = GET_DST_SQR(move);

	buff[0] = (srcsqr % 8) + 'a';
	buff[1] = (srcsqr / 8) + '1';
	buff[2] = (dstsqr % 8) + 'a';
	buff[3] = (dstsqr / 8) + '1';

	if (IS_PROM(move))
	{
		switch (GET_PROM_PIECE(move))
		{
			case BISHOP:
				buff[4] = 'b';
				break;
			case ROOK:
				buff[4] = 'r';
				break;
			case KNIGHT:
				buff[4] = 'n';
				break;
			case QUEEN:
				buff[4] = 'q';
				break;
			default:
				break;
		}
		buff[5] = '\0';
	}
	else
		buff[4] = '\0';
}

static Move parsemove(const Game* game, const char* str)
{
	int srcsqr = (str[0] - 'a') + (str[1] - '1') * 8;
	int dstsqr = (str[2] - 'a') + (str[3] - '1') * 8;

	BitBrd srcbbrd = sqr2bbrd(srcsqr);
	BitBrd dstbbrd = sqr2bbrd(dstsqr);

	const int enemy    = !game->who2move;
	const int movpiece = getpieceat(game->who2move, srcbbrd);

	if (movpiece == KING)
	{
		if (dstsqr == 2 && game->who2move == WHITE && CANCASTLE_WQ(g_gamestate))
			return SRC_SQR(4) | DST_SQR(2) | MOVE_F_ISCASTLEQ;
		if (dstsqr == 6 && game->who2move == WHITE && CANCASTLE_WK(g_gamestate))
			return SRC_SQR(4) | DST_SQR(6) | MOVE_F_ISCASTLEK;
		if (dstsqr == 58 && game->who2move == WHITE &&
				CANCASTLE_BQ(g_gamestate))
			return SRC_SQR(60) | DST_SQR(58) | MOVE_F_ISCASTLEQ;
		if (dstsqr == 62 && game->who2move == WHITE &&
				CANCASTLE_BK(g_gamestate))
			return SRC_SQR(60) | DST_SQR(62) | MOVE_F_ISCASTLEK;
	}

	Move move = SRC_SQR(srcsqr) | DST_SQR(dstsqr) | MOV_PIECE(movpiece);

	if (game->piecesof[enemy] & dstbbrd)
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

static void respond2ucinewgame(Game* game) {}

static void respond2position(Game* game, char* token)
{
	if (equals(token, "startpos"))
		reset_game(game);
	else if (equals(token, "fen"))
	{
		token = nexttok_untilend();
		parsefen(token);
	}
	else
		printf("%s\n", token);

	MoveList movelist;
	genmoves(game, game->who2move, &movelist);

	printf("Moves count: %d\n", movelist.cnt);
	for (int i = 0; i < movelist.cnt; i++)
	{
		char buff[6];
		printmove(buff, movelist.move[i]);
		printf("%s\n", buff);
	}
}

static void respond2debug(char* token)
{
	if (equals(token, "on"))
		g_ucidebug = 1;
	else if (equals(token, "off"))
		g_ucidebug = 0;
	else
		LOG("Invalid uci command: 'debug %s'", token);
}

static void respond2isready()
{
	printf("readyok\n");
	fflush(stdout);
}

static int next_cmd(Game* game, char* buff, int len)
{
	LOG("UCI: received command '%.*s'", len - 1, buff);
	char* token = strtok(buff, " \n");

	if (equals(token, "uci"))
		respond2uci();
	else if (equals(token, "ucinewgame"))
		respond2ucinewgame(game);
	else if (equals(token, "position"))
		respond2position(game, nexttok());
	else if (equals(token, "debug"))
		respond2debug(nexttok());
	else if (equals(token, "isready"))
		respond2isready();
	else if (equals(token, "quit"))
	{
		LOG("Closing");
		return 1;
	}
	else  // Unknown command
		LOG("Unsupported uci command: '%s'", token);

	return 0;
}

void uci_start()
{
	g_mode = MODE_UCI;
	respond2uci();

	Game game;

	size_t buffsize = 256;
	char*  buff	    = malloc(buffsize * sizeof(char));

	int quit = 0;
	int len  = 0;

	while (!quit)
	{
		if ((len = getline(&buff, &buffsize, stdin)) == -1)
			quit = 1;
		else
			quit = next_cmd(&game, buff, len);
	}

	free(buff);
}
