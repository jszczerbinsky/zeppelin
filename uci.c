#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#define TOK_DELIMS " \n"

#ifdef DEBUGON

static inline void dbgerr(const char* err)
{
	printf("%s\n", err);
	fflush(stdout);
}

#endif

static inline int equals(const char* s1, const char* s2)
{
	return strcmp(s1, s2) == 0;
}

static inline char* nexttok() { return strtok(NULL, TOK_DELIMS); }
static inline char* nexttok_untilend() { return strtok(NULL, "\0"); }

static Move parsemove(const char* str)
{
	int srcsqr = (str[0] - 'a') * 8 + (str[1] - '1');
	int dstsqr = (str[2] - 'a') * 8 + (str[3] - '1');

	// todo: detect piece, capture, ep, castle

	Move move = SRC_SQR(srcsqr) | DST_SQR(dstsqr);

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

static void printmove(char* buff, Move move)
{
	if (IS_CASTLEK(move))
	{
		// todo: detect side to move
	}
	else if (IS_CASTLEQ(move))
	{
		// todo: detect side to move
	}

	int srcsqr = GET_SRC_SQR(move);
	int dstsqr = GET_DST_SQR(move);

	buff[0] = (srcsqr / 8) + 'a';
	buff[1] = (srcsqr % 8) + '1';
	buff[2] = (dstsqr / 8) + 'a';
	buff[3] = (dstsqr % 8) + '1';

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

static void respond2uci()
{
	printf("id name testengine\n");
	printf("id author Jakub Szczerbinski\n");
	printf("uciok\n");
#ifdef DEBUGON
	printf("debug on\n");
#endif
	fflush(stdout);
}

static void respond2ucinewgame(Game* game) {}

static void respond2position(Game* game, char* token)
{
	if (equals(token, "startpos"))
		reset_game(game);
	else if (equals(token, "fen"))
	{
		token = nexttok_untilend();
		parse_fen(game, token);
	}
	else
		printf("%s\n", token);
}

static int next_cmd(Game* game, char* buff)
{
	char* token = strtok(buff, " \n");

	if (equals(token, "uci"))
		respond2uci();
	else if (equals(token, "ucinewgame"))
		respond2ucinewgame(game);
	else if (equals(token, "position"))
		respond2position(game, nexttok());
	else  // Unknown command
		return 1;

	return 0;
}

int uci_start()
{
	Game game;

	char*  buff;
	size_t buffsize = 256;

	int quit = 0;
	int err  = 0;

	while (!quit)
	{
		if (getline(&buff, &buffsize, stdin) == -1)
			quit = 1;
		else
			quit = err = next_cmd(&game, buff);
	}

	free(buff);

	return err;
}
