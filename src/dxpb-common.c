/*
 * dxpb-common.c
 *
 * Used to provide common functions to the dxpb-* family of user-facing
 * binaries
 */

#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include "dxpb-common.h"

#define VERSION "alpha-0.1.0"

void
prologue(const char *argv0)
{
	printf("DXPB: Distributed XBPS Package Builder\n%s %s\nCopyright 2016-2017, Toyam Cox\nThis is free software with ABSOLUTELY NO WARRANTY.\nFor license details, pass the -L flag\nCompiled: %s %s\n\n", argv0, VERSION, __DATE__, __TIME__);
}

void
print_license(void)
{
#include "license.inc"
	printf("%.*s", ___COPYING_len, ___COPYING);
}
