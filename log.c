#ifdef LOG_ENABLE

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"

const char* logfilename = "latest.log";

void initlog()
{
	FILE* logfile = fopen(logfilename, "w");
	fclose(logfile);
}

void printlog(const char* format, ...)
{
	FILE*   logfile = fopen(logfilename, "a");
	va_list args;
	va_start(args, format);
	vfprintf(logfile, format, args);
	va_end(args);
	fprintf(logfile, "\n");
	fclose(logfile);
}

#endif	// #ifdef LOG_ENABLE
