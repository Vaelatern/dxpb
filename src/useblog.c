#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>

#include "bstring.h"
#include "blog.c"

int main(void)
{
	char *name = strdup("/tmp/dxpb-blog-test-XXXXXX");
	assert(name);
	name = mkdtemp(name);
	char *logfile = bstring_add(name, "/capnp.packed", NULL, NULL);
	struct bworker wrkr = { .myaddr = 16,
				.addr = 0,
				.check = (uint32_t) 0x123456789abcdef,
				.mycheck = (uint32_t) 0xfedcba987654321,
				.arch = ARCH_ARMV6L,
				.hostarch = ARCH_ARMV6L_MUSL,
				.iscross = 1,
				.cost = 100};

	struct pkg a = {.name = "pkgA", .arch = ARCH_X86_64, .ver = "0.0.1"},
		   b = {.name = "pkgB", .arch = ARCH_X86_64_MUSL, .ver = "0.0.2"},
		   c = {.name = "pkgC", .arch = ARCH_ARMV6L, .ver = "0.0.4"},
		   d = {.name = "pkgD", .arch = ARCH_ARMV6L_MUSL, .ver = "0.0.3"};

	zlist_t *needs = zlist_new();
	zlist_append(needs, &a);
	zlist_append(needs, &b);
	zlist_append(needs, &c);
	zlist_append(needs, &d);

	blog_logging_on(1);
	blog_logfile(logfile);
	blog_pkgImported("Foo", "Bar", ARCH_NOARCH);
	blog_pkgAddedToGraph("Foo", "Bar", ARCH_X86_64);
	blog_graphSaved("1262ef2f058cdc342468f51f5cfe3451535c4d16");
	blog_pkgImportedForDeletion("Baz");
	blog_graphRead("1262ef2f058cdc342468f51f5cfe3451535c4d16");
	blog_logFiled("Foo", "Bar", ARCH_X86_64_MUSL);
	blog_pkgFetchStarting("Foo", "Bar", ARCH_ARMV7L);
	blog_pkgFetchComplete("Foo", "Bar", ARCH_ARMV7L);
	blog_workerAddedToGraphGroup(&wrkr);
	blog_workerMadeAvailable(&wrkr);
	blog_workerAssigned(&wrkr, "Faa", "Baa", ARCH_ARMV6L);
	blog_workerAssigning(&wrkr, "Faa", "Baa", ARCH_ARMV6L_MUSL);
	blog_logReceived(&wrkr, "Faa", "Baa", ARCH_ARMV6L);
	blog_logReceived(NULL, "Faa", "Baa", ARCH_ARMV7L);
	blog_workerAssignmentDone(&wrkr, "Faa", "Baa", ARCH_ARMV6L_MUSL, 2);

	blog_queueSelected(needs);

}
