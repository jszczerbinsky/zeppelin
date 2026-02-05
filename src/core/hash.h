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

#ifndef HASH_H
#define HASH_H

#include "bitboard.h"
#include "piece.h"

extern BitBrd hash_piecesqr[2][64][PIECE_MAX];
extern BitBrd hash_whitemove;
extern BitBrd hash_castle_wk;
extern BitBrd hash_castle_wq;
extern BitBrd hash_castle_bk;
extern BitBrd hash_castle_bq;
extern BitBrd hash_epfile[8];

void inithash(void);
BitBrd gethash(void);

#endif
