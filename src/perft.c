#include "main.h"

void perft(int depth, int* nodes, int* leafnodes)
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
			perft(depth - 1, nodes, leafnodes);
		unmakemove();
	}
}
