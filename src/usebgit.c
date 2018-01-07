#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include "../include/bgit.h"

int main(void)
{
	fprintf(stderr, "This relies on the caller's cwd being a packages repo\n");
	bgit_checkout_hash("./", "1caa35ab9baae8cfe2f52628b092fb577e4dbc2c");
}
