#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "main.h"

#define TOK_DELIMS " \n"

int g_ucidebug = 0;

static void respond2uci()
{
	printf("id name testengine\n");
	printf("id author Jakub Szczerbinski\n");
	printf("uciok\n");
	fflush(stdout);
}

static void respond2ucinewgame() {}

static void respond2position(char* token)
{
	if (equals(token, "startpos"))
	{
		token = nexttok();
		reset_game();
	}
	else if (equals(token, "fen"))
	{
		token = nexttok_untilend();
		token = parsefen(token);
	}

	if (token && equals(token, "moves"))
	{
		token = nexttok();
		while (token)
		{
			fflush(stdout);
			makemove(parsemove(token));
			token = nexttok();
		}
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

static void respond2go(char* token)
{
	if (equals(token, "perft"))
	{
		token = nexttok();
		if (token) perft(atoi(token));
	}
}

static int next_cmd(char* buff, int len)
{
	char* token = strtok(buff, " \n");
	if (!token) return 0;

	if (equals(token, "uci"))
		respond2uci();
	else if (equals(token, "ucinewgame"))
		respond2ucinewgame();
	else if (equals(token, "go"))
		respond2go(nexttok());
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
