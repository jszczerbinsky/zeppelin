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

#include "tt.h"
#include "../settings.h"

static TT *tt = NULL;
static size_t ttsize;
static size_t ttused = 0;

void ttinit(void) {
  ttsize = g_set.ttbytes / sizeof(TT);
  ttused = 0;
  tt = calloc(ttsize, sizeof(TT));
}

void ttfree(void) { free(tt); }

size_t getttsize(void) { return ttsize; }
size_t getttused(void) { return ttused; }

const TT *ttread(BitBrd hash, int depth, int searchid) {
  TT *ttentry = tt + (hash % ttsize);
  if (ttentry->used && ttentry->hash == hash && ttentry->depth >= depth) {
    ttentry->searchid = searchid;
    return ttentry;
  }
  return NULL;
}

void ttwrite(BitBrd hash, int type, int depth, int value, Move bestmove,
             int searchid) {
  TT *ttentry = tt + (hash % ttsize);

  if (!ttentry->used) {
    ttused++;
  } else if (ttentry->depth > 5 && depth + 2 <= ttentry->depth) {
    if (searchid - ttentry->searchid < 2) {
      return;
    }
  }

  ttentry->used = 1;
  ttentry->hash = hash;
  ttentry->type = type;
  ttentry->depth = depth;
  ttentry->value = value;
  ttentry->bestmove = bestmove;
  ttentry->searchid = searchid;
}
