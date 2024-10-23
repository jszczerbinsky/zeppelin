#include "main.h"

int g_mode = MODE_CLI;

int main(int argc, char** argv)
{
	LOG_INIT();
	cli_start();
	return 0;
}
