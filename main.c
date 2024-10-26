#include "main.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define ARG_NONE	 -1
#define ARG_HELP	 0
#define ARG_VER		 1
#define ARG_DUMP_PRECOMP 2
#define ARG_GEN_PRECOMP	 3

typedef struct
{
	const char* str;
	const char* desc;
	const int	argid;
} ArgDef;

static const char*  usagestr	  = "engine [option]";
static const ArgDef definedargs[] = {
	{"--help", "Display available options and usage", ARG_HELP},
	{"--version", "Display program version information", ARG_VER},
	{"--dump-precomp",
		"Dump precomputed engine values to a file",
		ARG_DUMP_PRECOMP},
	{"--gen-precomp", "Regenerate precomputed engine values", ARG_GEN_PRECOMP},
};
static const int definedargscnt = sizeof(definedargs) / sizeof(ArgDef);

int g_mode;

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
	}
}

static void printhelp()
{
	printf("Usage: %s\nOptions:\n", usagestr);

	for (int i = 0; i < definedargscnt; i++)
	{
		printf("  %*s", -20, definedargs[i].str);
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
	if (argc == 1)
	{
		if (!loadprecomp())
		{
			fprintf(stderr, "ERROR precomp file not found, aborting...\n");
			return 1;
		}

		choose_protocol();
		return 0;
	}
	else if (argc > 2)
	{
		fprintf(stderr, "Too many arguments\n");
		return 1;
	}

	switch (getarg(argv[1]))
	{
		case ARG_NONE:
			fprintf(stderr, "Unknown argument %s\n\n", argv[1]);
			printhelp();
			break;
		case ARG_HELP:
			printhelp();
			break;
		case ARG_VER:
			printf("Version: \n");
			break;
		case ARG_DUMP_PRECOMP:
			if (!loadprecomp())
			{
				fprintf(stderr, "ERROR precomp file not found, aborting...\n");
				return 1;
			}
			dumpprecomp();
			break;
		case ARG_GEN_PRECOMP:
			genprecomp();
			break;
	}

	return 0;
}
