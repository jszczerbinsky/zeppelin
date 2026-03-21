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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "core/hash.h"
#include "core/precomp.h"
#include "eval/nnue.h"
#include "iface/iface.h"
#include "search/search.h"
#include "settings.h"
#include "utils/strutils.h"

#define ARG_NONE -1
#define ARG_HELP 0
#define ARG_VER 1

typedef struct {
  const char *str;
  const char *fullstr;
  const char *desc;
  int subargscnt;
  const int argid;
} ArgDef;

static const char *usagestr = "zeppelin [option1] [option2]";
static const ArgDef definedargs[] = {
    {
        "--help",
        "--help",
        "Display available options and usage",
        0,
        ARG_HELP,
    },
    {
        "--version",
        "--version",
        "Display program version information",
        0,
        ARG_VER,
    },
};
static const int definedargscnt = sizeof(definedargs) / sizeof(ArgDef);

int g_mode = MODE_CLIARG;

Settings g_set = {0};

#ifdef DEBUG_INTERFACE
void (*g_printdbg)(const char *format, ...) = NULL;
#endif

static void choose_protocol(void) {
  const size_t buffsize = 256;
  char buff[buffsize];

  int quit = 0;

  while (!quit) {
    char *res = fgets(buff, (int)buffsize, stdin);

    if (res == NULL) {
      quit = 1;
    } else if (equals(buff, "quit\n")) {
      quit = 1;
    } else if (equals(buff, "uci\n")) {
      uci_start();
      quit = 1;
    }
#ifdef DEBUG_INTERFACE
    else if (equals(buff, "debug\n")) {
      debug_start();
      quit = 1;
    }
#endif
  }
}

static void printhelp(void) {
  printf("Usage: %s\nOptions:\n", usagestr);

  for (int i = 0; i < definedargscnt; i++) {
    printf("  %*s", -35, definedargs[i].fullstr);
    printf("%s\n", definedargs[i].desc);
  }
}

static int getarg(const char *argv) {
  for (int i = 0; i < definedargscnt; i++)
    if (equals(argv, definedargs[i].str))
      return definedargs[i].argid;
  return ARG_NONE;
}

int main(int argc, char **argv) {
  srand((unsigned int)time(NULL));
  inithash();
  nnue_load_weights();
  reset_hashtables();
  if (argc == 1) {
    if (!initprecomp()) {
      fprintf(stderr, "ERROR precomp file not found, aborting...\n");
      return 1;
    }

    choose_protocol();
    return 0;
  }

  int arg = getarg(argv[1]);

  switch (arg) {
  case ARG_NONE:
    fprintf(stderr, "Unknown argument %s\n\n", argv[1]);
    printhelp();
    return 1;
  default:
    break;
  }

  if (argc - 2 < definedargs[arg].subargscnt) {
    fprintf(stderr, "Missing argument after %s\n\n", argv[1]);
    printhelp();
    return 1;
  }

  if (!initprecomp()) {
    fprintf(stderr, "ERROR precomp file not found, aborting...\n");
    return 1;
  }

  switch (arg) {
  case ARG_HELP:
    printhelp();
    break;
  case ARG_VER:
    printf("Version: " PROGRAM_VERSION " for " TARGET_PLATFORM "\n");
    break;
  default:
    break;
  }

  return 0;
}
