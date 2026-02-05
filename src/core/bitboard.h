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

#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>
#include <stdlib.h>

typedef uint64_t BitBrd;

#define W_KING_SQR 4
#define B_KING_SQR 60

#define RANK_1 0xffULL
#define RANK_2 0xff00ULL
#define RANK_3 0xff0000ULL
#define RANK_4 0xff000000ULL
#define RANK_5 0xff00000000ULL
#define RANK_6 0xff0000000000ULL
#define RANK_7 0xff000000000000ULL
#define RANK_8 0xff00000000000000ULL

#define FILE_A 0x101010101010101ULL
#define FILE_B 0x202020202020202ULL
#define FILE_C 0x404040404040404ULL
#define FILE_D 0x808080808080808ULL
#define FILE_E 0x1010101010101010ULL
#define FILE_F 0x2020202020202020ULL
#define FILE_G 0x4040404040404040ULL
#define FILE_H 0x8080808080808080ULL

// rank - file + 7
#define DIAG_0 0x80ULL
#define DIAG_1 0x8040ULL
#define DIAG_2 0x804020ULL
#define DIAG_3 0x80402010ULL
#define DIAG_4 0x8040201008ULL
#define DIAG_5 0x804020100804ULL
#define DIAG_6 0x80402010080402ULL
#define DIAG_7 0x8040201008040201ULL
#define DIAG_8 0x4020100804020100ULL
#define DIAG_9 0x2010080402010000ULL
#define DIAG_10 0x1008040201000000ULL
#define DIAG_11 0x804020100000000ULL
#define DIAG_12 0x402010000000000ULL
#define DIAG_13 0x201000000000000ULL
#define DIAG_14 0x100000000000000ULL

// rank + file
#define ADIAG_0 0x1ULL
#define ADIAG_1 0x102ULL
#define ADIAG_2 0x10204ULL
#define ADIAG_3 0x1020408ULL
#define ADIAG_4 0x102040810ULL
#define ADIAG_5 0x10204081020ULL
#define ADIAG_6 0x1020408102040ULL
#define ADIAG_7 0x102040810204080ULL
#define ADIAG_8 0x204081020408000ULL
#define ADIAG_9 0x408102040800000ULL
#define ADIAG_10 0x810204080000000ULL
#define ADIAG_11 0x1020408000000000ULL
#define ADIAG_12 0x2040800000000000ULL
#define ADIAG_13 0x4080000000000000ULL
#define ADIAG_14 0x8000000000000000ULL

#define CENTER ((FILE_D | FILE_E) & (RANK_4 | RANK_5))
#define CENTER16                                                               \
  ((FILE_C | FILE_D | FILE_E | FILE_F) & (RANK_3 | RANK_4 | RANK_5 | RANK_6))

#define W_KINGSQR 4
#define B_KINGSQR 60

#define WK_ROOKSQR 7
#define BK_ROOKSQR 63
#define WQ_ROOKSQR 0
#define BQ_ROOKSQR 56

#define CASTLE_WK_KINGSQR 6
#define CASTLE_WK_ROOKSQR 5
#define CASTLE_BK_KINGSQR 62
#define CASTLE_BK_ROOKSQR 61
#define CASTLE_WQ_KINGSQR 2
#define CASTLE_WQ_ROOKSQR 3
#define CASTLE_BQ_KINGSQR 58
#define CASTLE_BQ_ROOKSQR 59

#define bbrd2sqr(bbrd) (__builtin_ffsll((long long)bbrd) - 1)

#define sqr2bbrd(sqr) (1ULL << (BitBrd)(sqr))

static inline int sqr2diag(int sqr) { return (sqr / 8) - (sqr % 8) + 7; }

static inline int sqr2antidiag(int sqr) { return (sqr / 8) + (sqr % 8); }

#define bbrdflipv(bbrd) (__builtin_bswap64(bbrd))

#define popcnt(bbrd) (__builtin_popcountll(bbrd))

#define nearby(bbrd)                                                           \
  ((bbrd << 8) | (bbrd >> 8) | ((bbrd & ~FILE_H) << 1) |                       \
   ((bbrd & ~FILE_A) >> 1) | ((bbrd & ~FILE_H) << 9) |                         \
   ((bbrd & ~FILE_H) >> 7) | ((bbrd & ~FILE_A) << 7) |                         \
   ((bbrd & ~FILE_A) >> 9))

/*static inline int popcnt(BitBrd bbrd) {
  int cnt = 0;

  while (bbrd) {
    cnt++;
    bbrd &= ~sqr2bbrd(bbrd2sqr(bbrd));
  }

  return cnt;
}*/

static inline BitBrd file2rank(BitBrd bbrd) { return (bbrd * DIAG_7) >> 56; }

static inline BitBrd rank2file(BitBrd bbrd) {
  return (bbrd * ADIAG_7) & FILE_A;
}

static inline BitBrd nextsubset(BitBrd subset, BitBrd set) {
  return (subset - set) & set;
}

static inline BitBrd rand64(void) {
  return (BitBrd)rand() | (((BitBrd)rand()) << 32);
}

#endif
