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

#ifndef MOVE_H
#define MOVE_H

#include <stdint.h>

typedef uint32_t Move;

// |31   21|20       18|17       15|14      12|11     6|5      0|
// | Type  | PromPiece | CaptPiece | MovPiece | DstSqr | SrcSqr |
// | 11bit |   3bit    |   3bit    |   3bit   |  6bit  |  6bit  |

#define MOVE_SRC_SQR_MASK 0x3FUL
#define MOVE_DST_SQR_MASK 0xFC0UL
#define MOVE_MOV_PIECE_MASK 0x7000UL
#define MOVE_CAPT_PIECE_MASK 0x38000UL
#define MOVE_PROM_PIECE_MASK 0x1C0000UL
#define MOVE_TYPE_MASK 0x3FE00000UL
#define MOVE_TYPE_NULL 0x0UL
#define MOVE_TYPE_CASTLEWQ 0x200000UL
#define MOVE_TYPE_CASTLEWK 0x400000UL
#define MOVE_TYPE_CASTLEBQ 0x600000UL
#define MOVE_TYPE_CASTLEBK 0x800000UL
#define MOVE_TYPE_NORMALPROM 0xA00000UL
#define MOVE_TYPE_NORMALCAPT 0xC00000UL
#define MOVE_TYPE_PROMCAPT 0xE00000UL
#define MOVE_TYPE_EP 0x1000000UL
#define MOVE_TYPE_DOUBLEPUSH 0x1200000UL
#define MOVE_TYPE_NORMAL 0x1400000UL

#define NULLMOVE 0U

#define GET_SRC_SQR(m) ((int)((m) & MOVE_SRC_SQR_MASK))
#define GET_DST_SQR(m) (((int)((m) & MOVE_DST_SQR_MASK) >> 6))
#define GET_MOV_PIECE(m) (((int)((m) & MOVE_MOV_PIECE_MASK) >> 12))
#define GET_CAPT_PIECE(m) (((int)((m) & MOVE_CAPT_PIECE_MASK) >> 15))
#define GET_PROM_PIECE(m) (((int)((m) & MOVE_PROM_PIECE_MASK) >> 18))
#define GET_TYPE(m) ((m) & MOVE_TYPE_MASK)
#define GET_CASLE_FLAGS(m) ((m) & MOVE_CASTLE_FLAGS_MASK)

#define SRC_SQR(s) ((Move)s)
#define DST_SQR(s) (((Move)(s)) << 6)
#define MOV_PIECE(p) (((Move)(p)) << 12)
#define CAPT_PIECE(p) (((Move)(p)) << 15)
#define PROM_PIECE(p) (((Move)(p)) << 18)

#define IS_PROM(m)                                                             \
  (GET_TYPE(m) == MOVE_TYPE_NORMALPROM || GET_TYPE(m) == MOVE_TYPE_PROMCAPT)
#define IS_CAPT(m)                                                             \
  (GET_TYPE(m) == MOVE_TYPE_NORMALCAPT || GET_TYPE(m) == MOVE_TYPE_PROMCAPT)

#define IS_CASTLE(m)                                                           \
  (GET_TYPE(m) >= MOVE_TYPE_CASTLEWQ && GET_TYPE(m) <= MOVE_TYPE_CASTLEBK)

#define IS_SILENT(m) (!IS_CAPT(m) && !IS_PROM(m))

#endif
