#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "main.h"

void fatalerr(const char* format, ...)
{
	printf("info string FATAL ERROR ");
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	putchar('\n');
	fflush(stdout);
	exit(1);
}
