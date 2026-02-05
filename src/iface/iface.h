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

#ifndef DEBUG_H
#define DEBUG_H

#ifdef DEBUG_INTERFACE
void debug_start(void);
extern void (*g_printdbg)(const char *format, ...);
#define PRINTDBG(...) (g_printdbg ? (*g_printdbg)(__VA_ARGS__) : (void)0)
#else
#define PRINTDBG(...)

#endif

extern int g_ucidebug;
void uci_start(void);

#define MODE_CLIARG 0
#define MODE_UCI 1
#define MODE_DEBUG 2

extern int g_mode;

#endif
