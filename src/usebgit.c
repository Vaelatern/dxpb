#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "../include/bgit.h"

int main(void)
{
	char *name = strdup("/tmp/dxpb-harness-test-XXXXXX");
	assert(name);
	name = mkdtemp(name);
	char *url = "https://github.com/voidlinux/void-packages.git";
	bgit_just_ff(url, name);
	bgit_checkout_hash(name, "1caa35ab9baae8cfe2f52628b092fb577e4dbc2c");
}
