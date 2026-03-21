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

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>

#include <string.h>

#include "bitboard.h"
#include "game.h"
#include "precomp.h"

#ifdef BIT_NONE
extern const unsigned char _binary_precomp_magic_bin_start[]
    __attribute__((aligned(8)));
extern const unsigned char _binary_precomp_magic_bin_end[];
static const unsigned char *_precomp_data = _binary_precomp_magic_bin_start;
#endif
#ifdef BIT_BMI2
extern const unsigned char _binary_precomp_pext_bin_start[]
    __attribute__((aligned(8)));
extern const unsigned char _binary_precomp_pext_bin_end[];
static const unsigned char *_precomp_data = _binary_precomp_pext_bin_start;
#endif

PrecompTable g_precomp;

int initprecomp(void) {
  const unsigned char *ptr = _precomp_data;

  g_precomp.knightmask = (const BitBrd *)ptr;
  ptr += sizeof(BitBrd) * 64;
  g_precomp.kingmask = (const BitBrd *)ptr;
  ptr += sizeof(BitBrd) * 64;
  g_precomp.bishoppremask = (const BitBrd *)ptr;
  ptr += sizeof(BitBrd) * 64;
  g_precomp.bishoppostmask = (const BitBrd *)ptr;
  ptr += sizeof(BitBrd) * 64;
  g_precomp.rookpremask = (const BitBrd *)ptr;
  ptr += sizeof(BitBrd) * 64;
  g_precomp.rookpostmask = (const BitBrd *)ptr;
  ptr += sizeof(BitBrd) * 64;
  g_precomp.queenpremask = (const BitBrd *)ptr;
  ptr += sizeof(BitBrd) * 64;
  g_precomp.queenpostmask = (const BitBrd *)ptr;
  ptr += sizeof(BitBrd) * 64;
  g_precomp.pawnattackmask[WHITE] = (const BitBrd *)ptr;
  ptr += sizeof(BitBrd) * 64;
  g_precomp.pawnattackmask[BLACK] = (const BitBrd *)ptr;
  ptr += sizeof(BitBrd) * 64;

#ifdef BIT_NONE
  g_precomp.rookmagicshift = (const uint8_t *)ptr;
  ptr += sizeof(uint8_t) * 64;
  g_precomp.bishopmagicshift = (const uint8_t *)ptr;
  ptr += sizeof(uint8_t) * 64;

  while ((uintptr_t)ptr % alignof(BitBrd) != 0)
    ptr += 1;

  g_precomp.rookmagic = (const BitBrd *)ptr;
  ptr += sizeof(BitBrd) * 64;
  g_precomp.bishopmagic = (const BitBrd *)ptr;
  ptr += sizeof(BitBrd) * 64;

  for (int i = 0; i < 64; i++) {
    size_t cnt = ((size_t)1) << (64 - (int)g_precomp.rookmagicshift[i]);

    g_precomp.rookmagicmoves[i] = (const BitBrd *)ptr;
    ptr += sizeof(BitBrd) * cnt;
  }
  for (int i = 0; i < 64; i++) {
    size_t cnt = ((size_t)1) << (64 - (int)g_precomp.bishopmagicshift[i]);

    g_precomp.bishopmagicmoves[i] = (const BitBrd *)ptr;
    ptr += sizeof(BitBrd) * cnt;
  }
#endif
#ifdef BIT_BMI2
  g_precomp.rookpextsize = (const uint16_t *)ptr;
  ptr += sizeof(uint16_t) * 64;
  g_precomp.bishoppextsize = (const uint16_t *)ptr;
  ptr += sizeof(uint16_t) * 64;

  while ((uintptr_t)ptr % alignof(BitBrd) != 0)
    ptr += 1;

  for (int i = 0; i < 64; i++) {
    g_precomp.rookpextmoves[i] = (const BitBrd *)ptr;
    ptr += sizeof(BitBrd) * (size_t)g_precomp.rookpextsize[i];
  }
  for (int i = 0; i < 64; i++) {
    g_precomp.bishoppextmoves[i] = (const BitBrd *)ptr;
    ptr += sizeof(BitBrd) * (size_t)g_precomp.bishoppextsize[i];
  }
#endif

  return 1;
}
