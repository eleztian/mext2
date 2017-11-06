#include <stdio.h>
#include "mext2.h"
#include "mshell.h"
int main(int argc, char *argv[])
{
	initialize_memory();

	mshell_init();
	return 0;
}