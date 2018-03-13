/*
 * bvirtpkg.c
 *
 * Simple module to read and manage virtual packages
 *
 */
#define _POSIX_C_SOURCE 200809L

#include <czmq.h>
#include "dxpb.h"
#include "bstring.h"
#include "bwords.h"
#include "bvirtpkg.h"

bwords
bvirtpkg_read(const char *repopath)
{
	bwords retVal = bwords_new();

	char *filepath = bstring_add(bstring_add(NULL, repopath, NULL, NULL),
			"/etc/defaults.virtual", NULL, NULL);
	assert(filepath);

	FILE *handle = fopen(filepath,"r");
	if(!handle)
		return NULL;
	FREE(filepath);

	char line[2050],virtualpkg[1025],resolvesto[1025];
	while (fgets(line, sizeof line, handle)) {
		if (*line == '#') continue; /* ignore comment line */
		if (sscanf(line, "%1024s %1024s", virtualpkg, resolvesto) == 2) {
			bwords_append_word(retVal, virtualpkg, 0);
			bwords_append_word(retVal, resolvesto, 0);
		}
	}

	return retVal;
}

zhash_t *
bvirtpkg_from_words(const bwords input)
{
	zhash_t *retVal = zhash_new();

	if (input->num_words % 2 != 0) {
		fprintf(stderr, "Virtualpkgs were not in even pairs, thus are invalid\n");
		return NULL;
	}

	for (unsigned long i = 0; i < input->num_words - 1; i += 2) {
		char *tmp = strdup(input->words[i+1]);
		if (!tmp)
			exit(ERR_CODE_NOMEM);
		zhash_insert(retVal, input->words[i], tmp);
	}

	return retVal;
}
