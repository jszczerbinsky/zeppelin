#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define ARG_NONE	 -1
#define ARG_HELP	 0
#define ARG_VER		 1
#define ARG_DUMP_PRECOMP 2
#define ARG_GEN_PRECOMP	 3
#define ARG_HUNT_MAGIC	 4
#define ARG_USE_MAGIC	 5
#define ARG_DUMP_POS	 6

typedef struct
{
	const char* str;
	const char* fullstr;
	const char* desc;
	int		subargscnt;
	const int	argid;
} ArgDef;

static const char*  usagestr	  = "engine [option1] [option2]";
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
	{
		"--gen-precomp",
		"--gen-precomp",
		"Regenerate precomputed engine values",
		0,
		ARG_GEN_PRECOMP,
	},
	{
		"--dump-precomp",
		"--dump-precomp",
		"Dump precomputed engine values to a file",
		0,
		ARG_DUMP_PRECOMP,
	},
	{
		"--hunt-magic",
		"--hunt-magic",
		"Try to find better magic numbers",
		0,
		ARG_HUNT_MAGIC,
	},
	{
		"--use-magic",
		"--use-magic <magic number HEX>",
		"Use a specific magic number",
		1,
		ARG_USE_MAGIC,
	},
	{
		"--dump-position",
		"--dump-position \"<FEN string>\"",
		"Dump information about a specified FEN position",
		1,
		ARG_DUMP_POS,
	},

};
static const int definedargscnt = sizeof(definedargs) / sizeof(ArgDef);

int g_mode = MODE_CLIARG;

static void choose_protocol()
{
	size_t buffsize = 256;
	char*  buff	    = malloc(buffsize * sizeof(char));

	int quit = 0;
	int len  = 0;

	while (!quit)
	{
		len = getline(&buff, &buffsize, stdin);

		if (len == -1)
		{
			quit = 1;
		}
		else if (equals(buff, "quit\n"))
		{
			quit = 1;
		}
		else if (equals(buff, "uci\n"))
		{
			free(buff);
			uci_start();
			quit = 1;
		}
#ifdef DEBUG_INTERFACE
		else if (equals(buff, "debug\n"))
		{
			free(buff);
			debug_start();
			quit = 1;
		}
#endif
	}
}

static void printhelp()
{
	printf("Usage: %s\nOptions:\n", usagestr);

	for (int i = 0; i < definedargscnt; i++)
	{
		printf("  %*s", -35, definedargs[i].fullstr);
		printf("%s\n", definedargs[i].desc);
	}
}

static int getarg(const char* argv)
{
	for (int i = 0; i < definedargscnt; i++)
		if (equals(argv, definedargs[i].str)) return definedargs[i].argid;
	return ARG_NONE;
}

int main(int argc, char** argv)
{
	srand(time(NULL));
	if (argc == 1)
	{
		if (!loadprecomp())
		{
			fprintf(stderr, "ERROR precomp file not found, aborting...\n");
			return 1;
		}

		choose_protocol();
		freeprecomp();
		return 0;
	}

	int arg = getarg(argv[1]);

	switch (arg)
	{
		case ARG_NONE:
			fprintf(stderr, "Unknown argument %s\n\n", argv[1]);
			printhelp();
			return 1;
		case ARG_GEN_PRECOMP:
			genprecomp();
			freeprecomp();
			return 0;
		default:
			break;
	}

	if (argc - 2 < definedargs[arg].subargscnt)
	{
		fprintf(stderr, "Missing argument after %s\n\n", argv[1]);
		printhelp();
		return 1;
	}

	if (!loadprecomp())
	{
		fprintf(stderr, "ERROR precomp file not found, aborting...\n");
		return 1;
	}

	switch (arg)
	{
		case ARG_HELP:
			printhelp();
			break;
		case ARG_VER:
			printf("Version: \n");
			break;
		case ARG_DUMP_PRECOMP:
			dumpprecomp();
			break;
		case ARG_DUMP_POS:
			parsefen(argv[2]);
			dumppos();
			break;
		case ARG_HUNT_MAGIC:
			huntmagic();
			break;
		case ARG_USE_MAGIC:
			usemagic(argv[2]);
			break;
		default:
			break;
	}

	freeprecomp();

	return 0;
}
