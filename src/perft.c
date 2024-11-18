#include <stdio.h>

#include "main.h"

static void perft_depth(int depth, int* nodes, int* leafnodes)
{
	if (depth == 0)
	{
		(*nodes)++;
		(*leafnodes)++;
		return;
	}

	MoveList movelist;
	genmoves(g_game.who2move, &movelist);

	if (movelist.cnt == 0)
	{
		(*nodes)++;
		(*leafnodes)++;
		return;
	}

	for (int i = 0; i < movelist.cnt; i++)
	{
		makemove(movelist.move[i]);

		if (!sqr_attackedby(
					g_game.who2move, bbrd2sqr(g_game.pieces[!g_game.who2move][KING])
					))
			perft_depth(depth - 1, nodes, leafnodes);
		unmakemove();
	}
}

void perft(int depth)
{
	MoveList movelist;
	genmoves(g_game.who2move, &movelist);

	int nodes = 0;

	for (int i = 0; i < movelist.cnt; i++)
	{
		Game backup;
		memcpy(&backup, &g_game, sizeof(Game));

		int leafnodes = 0;
		makemove(movelist.move[i]);
		char buff[6];
		if (!sqr_attackedby(
					g_game.who2move, bbrd2sqr(g_game.pieces[!g_game.who2move][KING])
					))
		{
			perft_depth(depth - 1, &nodes, &leafnodes);
			move2str(buff, movelist.move[i]);
			printf("%s: %d\n", buff, leafnodes);
		}

		unmakemove();
	}
	printf("\nTotal: %d\n", nodes);
	fflush(stdout);
}
