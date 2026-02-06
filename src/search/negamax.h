/*
 * Zeppelin chess engine.
 *
 * Copyright (C) 2024-2026 Jakub Szczerbiński <jszczerbinsky2@gmail.com>
 *
 * Zeppelin is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef NEGAMAX_H
#define NEGAMAX_H

#include "../core/movelist.h"
#include "search.h"

#define NODE_INSIDEWND 0
#define NODE_FAILH 1
#define NODE_FAILL 2
#define NODE_GAMEFINISHED 3
#define NODE_SELECTIVELY_PRUNED 4

int negamax(Search *s, int alpha, int beta, int depthleft, MoveList *pvdest,
            int ispv);

#endif
