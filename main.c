#include "main.h"

#include <unistd.h>

int main(int argc, char** argv)
{
	LOG_INIT();
	uci_start();
	return 0;
}
