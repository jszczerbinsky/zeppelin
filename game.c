#include <stdio.h>
#include <string.h>

#include "main.h"

#define FEN_TOK_DELIMS " /\n"

static inline char* fen_nexttok() { return strtok(NULL, FEN_TOK_DELIMS); }

void update_bbrds(Game* game)
{
	for (int p = 0; p < 2; p++)
	{
		game->playerpieces[p] = 0;
		for (int i = 0; i < PIECE_MAX; i++)
			game->playerpieces[p] |= game->pieces[p][i];
	}

	game->allpieces = game->playerpieces[WHITE] | game->playerpieces[BLACK];
}

void reset_game(Game* game)
{
	game->who2move	  = WHITE;
	game->movelist.plycnt = 0;

	game->brdstate[0].halfmove = 0;
	game->brdstate[0].fullmove = 0;
	game->brdstate[0].flags    = GAME_F_DEFAULT;

	game->pieces[WHITE][PAWN]	= 0xff;
	game->pieces[WHITE][KING]	= 0x10;
	game->pieces[WHITE][KNIGHT] = 0x42;
	game->pieces[WHITE][BISHOP] = 0x24;
	game->pieces[WHITE][ROOK]	= 0x81;
	game->pieces[WHITE][QUEEN]	= 0x8;

	game->pieces[BLACK][PAWN]	= 0xff000000000000;
	game->pieces[BLACK][KING]	= 0x1000000000000000;
	game->pieces[BLACK][KNIGHT] = 0x4200000000000000;
	game->pieces[BLACK][BISHOP] = 0x2400000000000000;
	game->pieces[BLACK][ROOK]	= 0x8100000000000000;
	game->pieces[BLACK][QUEEN]	= 0x800000000000000;

	update_bbrds(game);
}

void parse_fen(Game* game, char* fen)
{
	char* token = strtok(fen, FEN_TOK_DELIMS);

	int rank = 7;

	while (rank >= 0)
	{
		printf("%s\n", token);

		int file = 7;
		while (file >= 0)
		{
			const char* ptr = token;
			int		sqr = rank * 8 + file;
			if (*ptr >= '1' && *ptr <= '8')
				file -= *ptr - '0';
			else
			{
				switch (*ptr)
				{
					case 'p':
						game->pieces[BLACK][PAWN] |= sqr2bbrd(sqr);
						break;
					case 'k':
						game->pieces[BLACK][KING] |= sqr2bbrd(sqr);
						break;
					case 'n':
						game->pieces[BLACK][KNIGHT] |= sqr2bbrd(sqr);
						break;
					case 'b':
						game->pieces[BLACK][BISHOP] |= sqr2bbrd(sqr);
						break;
					case 'r':
						game->pieces[BLACK][ROOK] |= sqr2bbrd(sqr);
						break;
					case 'q':
						game->pieces[BLACK][QUEEN] |= sqr2bbrd(sqr);
						break;
					case 'P':
						game->pieces[WHITE][PAWN] |= sqr2bbrd(sqr);
						break;
					case 'K':
						game->pieces[WHITE][KING] |= sqr2bbrd(sqr);
						break;
					case 'N':
						game->pieces[WHITE][KNIGHT] |= sqr2bbrd(sqr);
						break;
					case 'B':
						game->pieces[WHITE][BISHOP] |= sqr2bbrd(sqr);
						break;
					case 'R':
						game->pieces[WHITE][ROOK] |= sqr2bbrd(sqr);
						break;
					case 'Q':
						game->pieces[WHITE][QUEEN] |= sqr2bbrd(sqr);
						break;
					default:
						break;
				}
				ptr++;
				file--;
			}
		}
		rank--;
		token = fen_nexttok();
	}

	printf("%lx\n", game->pieces[WHITE][PAWN]);
	printf("%lx\n", game->pieces[BLACK][KING]);
	printf("%lx\n", game->pieces[BLACK][PAWN]);
	// printf("%s\n", ptr);
}

void makemove(Game* game, Move move) {}

void undomove(Game* game) {}
