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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <stddef.h>

typedef struct {
  int disbl_ab;
  int disbl_quiescence;
  int disbl_nmp;
  int disbl_tt;
  int disbl_killer;
  int disbl_pvs;
  int disbl_lmr;
  int disbl_aspwnd;
  int disbl_delta;
  int disbl_fp;

  int gen_evals;

  int print_currline;

  size_t ttbytes;
} Settings;

extern Settings g_set;

#endif
