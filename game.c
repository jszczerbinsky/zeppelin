#include <stdio.h>
#include <string.h>

#include "main.h"

#define FEN_TOK_DELIMS " /\n"

Game	   g_game;
GameState* g_gamestate;

static inline char* fen_nexttok() { return strtok(NULL, FEN_TOK_DELIMS); }

static inline void update_gamestate()
{
	g_gamestate = g_game.brdstate + g_game.movelist.cnt;
}

void update_game()
{
	for (int p = 0; p < 2; p++)
	{
		g_game.piecesof[p] = 0;
		for (int i = 0; i < PIECE_MAX; i++)
		{
			g_game.piecesof[p] |= g_game.pieces[p][i];
		}
	}

	for (int i = 0; i < PIECE_MAX; i++)
		g_game.pieces[ANY][i] =
			g_game.pieces[WHITE][i] | g_game.pieces[BLACK][i];

	g_game.piecesof[ANY] = g_game.piecesof[WHITE] | g_game.piecesof[BLACK];

	update_gamestate();
}

int getpieceat(int color, BitBrd bbrd)
{
	for (int p = 0; p < PIECE_MAX; p++)
		if (g_game.pieces[color][p] & bbrd) return p;
	return -1;
}

void reset_game()
{
	g_game.who2move	= WHITE;
	g_game.movelist.cnt = 0;

	g_game.brdstate[0].halfmove = 0;
	g_game.brdstate[0].fullmove = 0;
	g_game.brdstate[0].flags	= GAME_F_DEFAULT;

	g_game.pieces[WHITE][PAWN]	 = 0xff00ULL;
	g_game.pieces[WHITE][KING]	 = 0x10ULL;
	g_game.pieces[WHITE][KNIGHT] = 0x42ULL;
	g_game.pieces[WHITE][BISHOP] = 0x24ULL;
	g_game.pieces[WHITE][ROOK]	 = 0x81ULL;
	g_game.pieces[WHITE][QUEEN]	 = 0x8ULL;

	g_game.pieces[BLACK][PAWN]	 = 0xff000000000000ULL;
	g_game.pieces[BLACK][KING]	 = 0x1000000000000000ULL;
	g_game.pieces[BLACK][KNIGHT] = 0x4200000000000000ULL;
	g_game.pieces[BLACK][BISHOP] = 0x2400000000000000ULL;
	g_game.pieces[BLACK][ROOK]	 = 0x8100000000000000ULL;
	g_game.pieces[BLACK][QUEEN]	 = 0x800000000000000ULL;

	update_game();
}

void getfen(char* buff) { strcpy(buff, "not supported yet"); }

int parsefen(char* fen)
{
	memset(&g_game, 0, sizeof(Game));
	update_gamestate();

	char* token = strtok(fen, FEN_TOK_DELIMS);

	if (!token)
	{
		return 0;
	}

	int rank = 7;

	while (rank >= 0)
	{
		int file = 0;
		while (file < 8)
		{
			if (!token)
			{
				return 0;
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
						g_game.pieces[BLACK][PAWN] |= sqr2bbrd(sqr);
						break;
					case 'k':
						g_game.pieces[BLACK][KING] |= sqr2bbrd(sqr);
						break;
					case 'n':
						g_game.pieces[BLACK][KNIGHT] |= sqr2bbrd(sqr);
						break;
					case 'b':
						g_game.pieces[BLACK][BISHOP] |= sqr2bbrd(sqr);
						break;
					case 'r':
						g_game.pieces[BLACK][ROOK] |= sqr2bbrd(sqr);
						break;
					case 'q':
						g_game.pieces[BLACK][QUEEN] |= sqr2bbrd(sqr);
						break;
					case 'P':
						g_game.pieces[WHITE][PAWN] |= sqr2bbrd(sqr);
						break;
					case 'K':
						g_game.pieces[WHITE][KING] |= sqr2bbrd(sqr);
						break;
					case 'N':
						g_game.pieces[WHITE][KNIGHT] |= sqr2bbrd(sqr);
						break;
					case 'B':
						g_game.pieces[WHITE][BISHOP] |= sqr2bbrd(sqr);
						break;
					case 'R':
						g_game.pieces[WHITE][ROOK] |= sqr2bbrd(sqr);
						break;
					case 'Q':
						g_game.pieces[WHITE][QUEEN] |= sqr2bbrd(sqr);
						break;
					default:
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
		return 0;
	}

	if (token[0] == 'w')
		g_game.who2move = WHITE;
	else if (token[0] == 'b')
		g_game.who2move = BLACK;

	token = fen_nexttok();
	if (!token)
	{
		return 0;
	}

	while (*token && *token != '-')
	{
		switch (*token)
		{
			case 'k':
				g_gamestate->flags |= GAME_F_CANCASTLE_BK;
				break;
			case 'q':
				g_gamestate->flags |= GAME_F_CANCASTLE_BQ;
				break;
			case 'K':
				g_gamestate->flags |= GAME_F_CANCASTLE_WK;
				break;
			case 'Q':
				g_gamestate->flags |= GAME_F_CANCASTLE_WQ;
				break;
			default:
				break;
		}
		token++;
	}

	token = fen_nexttok();
	if (!token)
	{
		return 0;
	}

	if (*token != '-')
	{
		int file = token[0] - 'a';
		int rank = token[1] - '0';

		g_gamestate->epbbrd = file + rank * 8;
	}

	token = fen_nexttok();
	if (!token)
	{
		return 0;
	}

	int halfmov = 0;
	while (*token)
	{
		if (*token < '0' || *token > '9') halfmov *= 10;
		halfmov += *token - '0';
		token++;
	}
	g_gamestate->halfmove = halfmov;

	token = fen_nexttok();
	if (!token)
	{
		return 0;
	}

	int fullmov = 0;
	while (*token)
	{
		if (*token < '0' || *token > '9') fullmov *= 10;
		fullmov += *token - '0';
		token++;
	}
	g_gamestate->fullmove = fullmov;

	update_game();

	return 1;
}

void move2str(char* buff, Move move)
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

void makemove(Move move) {}

void undomove() {}
