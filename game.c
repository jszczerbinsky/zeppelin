#include <stdio.h>
#include <string.h>

#include "main.h"

#define FEN_TOK_DELIMS " /\n"

static inline char* fen_nexttok() { return strtok(NULL, FEN_TOK_DELIMS); }

void update_bbrds(Game* game)
{
	for (int p = 0; p < 2; p++)
	{
		game->piecesof[p] = 0;
		for (int i = 0; i < PIECE_MAX; i++)
			game->piecesof[p] |= game->pieces[p][i];
	}

	game->piecesof[ANY] = game->piecesof[WHITE] | game->piecesof[BLACK];
}

int getpieceat(const Game* game, int color, BitBrd bbrd)
{
	for (int p = 0; p < PIECE_MAX; p++)
		if (game->pieces[color][p] & bbrd) return p;
	return -1;
}

void reset_game(Game* game)
{
	game->who2move     = WHITE;
	game->movelist.cnt = 0;

	game->brdstate[0].halfmove = 0;
	game->brdstate[0].fullmove = 0;
	game->brdstate[0].flags    = GAME_F_DEFAULT;

	game->pieces[WHITE][PAWN]	= 0xff00ULL;
	game->pieces[WHITE][KING]	= 0x10ULL;
	game->pieces[WHITE][KNIGHT] = 0x42ULL;
	game->pieces[WHITE][BISHOP] = 0x24ULL;
	game->pieces[WHITE][ROOK]	= 0x81ULL;
	game->pieces[WHITE][QUEEN]	= 0x8ULL;

	game->pieces[BLACK][PAWN]	= 0xff000000000000ULL;
	game->pieces[BLACK][KING]	= 0x1000000000000000ULL;
	game->pieces[BLACK][KNIGHT] = 0x4200000000000000ULL;
	game->pieces[BLACK][BISHOP] = 0x2400000000000000ULL;
	game->pieces[BLACK][ROOK]	= 0x8100000000000000ULL;
	game->pieces[BLACK][QUEEN]	= 0x800000000000000ULL;

	update_bbrds(game);
}

void parse_fen(Game* game, char* fen)
{
	LOG("FEN: Starting parsing");

	memset(game, 0, sizeof(Game));

	char* token = strtok(fen, FEN_TOK_DELIMS);

	if (!token)
	{
		LOG("FEN: FEN is empty");
		return;
	}

	int rank = 7;

	while (rank >= 0)
	{
		int file = 0;
		while (file < 8)
		{
			if (!token)
			{
				LOG("FEN: Couldn't parse piece information");
				return;
			}

			int sqr = rank * 8 + file;
			if (*token >= '1' && *token <= '8')
			{
				file += *token - '0';
				token++;
			}
			else
			{
				switch (*token)
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
						LOG("FEN: Incorrect piece char '%c'", *token);
						break;
				}
				token++;
				file++;
			}
		}
		rank--;
		token = fen_nexttok();
	}

	if (!token)
	{
		LOG("FEN: side to move missing");
		return;
	}

	if (token[0] == 'w')
		game->who2move = WHITE;
	else if (token[0] == 'b')
		game->who2move = BLACK;
	else
		LOG("FEN: Incorrect side to move '%c'", token[0]);

	token = fen_nexttok();
	if (!token)
	{
		LOG("FEN: castling flags missing");
		return;
	}

	while (*token && *token != '-')
	{
		switch (*token)
		{
			case 'k':
				GET_CURR_STATE(game)->flags |= GAME_F_CANCASTLE_BK;
				break;
			case 'q':
				GET_CURR_STATE(game)->flags |= GAME_F_CANCASTLE_BQ;
				break;
			case 'K':
				GET_CURR_STATE(game)->flags |= GAME_F_CANCASTLE_WK;
				break;
			case 'Q':
				GET_CURR_STATE(game)->flags |= GAME_F_CANCASTLE_WQ;
				break;
			default:
				LOG("FEN: Incorrect castle flag '%c'", *token);
				break;
		}
		token++;
	}

	token = fen_nexttok();
	if (!token)
	{
		LOG("FEN: EP square missing");
		return;
	}

	if (*token != '-')
	{
		int file = token[0] - 'a';
		int rank = token[1] - '0';

		GET_CURR_STATE(game)->epbbrd = file + rank * 8;
	}

	token = fen_nexttok();
	if (!token)
	{
		LOG("FEN: halfmove clock missing");
		return;
	}

	int halfmov = 0;
	while (*token)
	{
		if (*token < '0' || *token > '9')
			LOG("FEN: Excepted a number in halfmove counter");
		halfmov *= 10;
		halfmov += *token - '0';
		token++;
	}
	GET_CURR_STATE(game)->halfmove = halfmov;

	token = fen_nexttok();
	if (!token)
	{
		LOG("FEN: fullmove clock missing");
		return;
	}

	int fullmov = 0;
	while (*token)
	{
		if (*token < '0' || *token > '9')
			LOG("FEN: Excepted a number in fullmove counter");
		fullmov *= 10;
		fullmov += *token - '0';
		token++;
	}
	GET_CURR_STATE(game)->fullmove = fullmov;

	update_bbrds(game);

	LOG("FEN: Finished parsing");
}

void makemove(Game* game, Move move) {}

void undomove(Game* game) {}
