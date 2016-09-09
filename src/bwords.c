/*
 * bwords.c
 *
 * A module for dealing with bwords, a char** type to represent multiple
 * strings that should be moved together, such as a list of package names as
 * dependencies.
 */
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "dxpb.h"
#include "bstring.h"
#include "bwords.h"

/* Important for the definition of units */
/* 037 = 31 dec = ASCII unit separator */
#define UNIT_SEPARATOR "\37"

/* Local prototypes */
static int	qsort_strcmp(const void *, const void *);

/* Private */
static int
qsort_strcmp(const void *a, const void *b)
{
	if (a == b) return 0;
	if (a == NULL) return 1;
	if (b == NULL) return -1;
	return strcmp(*((char **)a), *((char **)b));
}

/* API */
bwords
bwords_new(void)
{
	struct bwords *retVal = malloc(sizeof(struct bwords));
	if (retVal) {
		retVal->num_words = 0;
		retVal->num_alloc = 1;
		retVal->words = calloc(1, sizeof(char *));
		if (retVal->words)
			return retVal;
	}
	perror("Was not able to allocate a new set of words");
	exit(ERR_CODE_NOMEM);
}

void
bwords_destroy(bwords *victim, char also_inside)
{
	if (!victim)
		return;
	if (!(*victim))
		return;
	if (!((*victim)->words))
		return;
	if (also_inside)
		for (int i = 0; (*victim)->words[i] != NULL; i++)
			free((void *)((*victim)->words[i]));
	free((*victim)->words);
	(*victim)->words = NULL;
	free(*victim);
	*victim = NULL;
}

bwords
bwords_append_word(bwords words, const char *word, char zero_copy)
{
	const char *tmp;
	if (words == NULL)
		words = bwords_new();

	/* In this section, we obtain a tmp variable for the word to add
	 * Allocations included, this should not be complicated and simply
	 * give us a pointer to a known good string. */
	if (word == NULL || word[0] == '\0') { /* Special case to pair with normalizing array */
		tmp = NULL;
	} else if (!zero_copy) {
		if ((tmp = strdup(word)) == NULL) {
			perror("Could not allocate memory");
			exit(ERR_CODE_NOMEM);
		}
	} else {
		tmp = word;
	}

	if (tmp != NULL)
		words->num_words++;
	/* Now we do reallocation to get the correct size */
	if (tmp == NULL) { /* Special case to find max size */
		words->num_alloc = words->num_words + 1;
		words->words = realloc(words->words, words->num_alloc*sizeof(char **));
		if (words->words == NULL) {
			perror("WTF. Shrinking list of strings fails? WTF?");
			exit(ERR_CODE_BADDOBBY);
		}
	} else if (words->num_words + 1 > words->num_alloc) {
		if (words->num_alloc == 0)
			words->num_alloc = 1;
		while (words->num_words + 1 > words->num_alloc) {
			words->num_alloc *= 2;
		}
		words->words = realloc(words->words, words->num_alloc*sizeof(char **));
		if (words == NULL) {
			perror("Out of memory for list of strings");
			exit(ERR_CODE_NOMEM);
		}
	}
	/* If this is the sort of thing where we do writing, we do writing */
	if (tmp != NULL) {
		words->words[words->num_words - 1] = tmp;
	}
	/* Either way, by convention we have a NULL at the end */
	words->words[words->num_words] = NULL;
	/* And return */
	return words;
}

bwords
bwords_merge_words(bwords initial, bwords toadd)
{
	if (initial == NULL)
		return bwords_append_word(toadd, NULL, 0);
	if (toadd == NULL)
		return bwords_append_word(initial, NULL, 0);
	for (size_t i = 0; toadd->words[i] != NULL; i++) {
		initial = bwords_append_word(initial, toadd->words[i], 1);
	}
	bwords_destroy(&toadd, 0);
	return initial;
}

char *
bwords_to_units(bwords words)
{
	uint32_t alloced, len;
	char *retVal;
	retVal = NULL;
	alloced = len = 0;
	for (size_t i = 0; words != NULL && words->words != NULL &&
				words->words[i] != NULL; i++) {
		retVal = bstring_add(retVal, words->words[i], &alloced, &len);
		if (words->words[i+1] != NULL)
			retVal = bstring_add(retVal, UNIT_SEPARATOR, &alloced, &len);
		if (retVal == NULL) {
			perror("Couldn't do an add to string");
			exit(ERR_CODE_NOMEM);
		}
	}
	retVal = realloc(retVal, (len*sizeof(char))+1);
	if (retVal == NULL) {
		perror("WTF. Shrinking string fails? WTF?");
		exit(ERR_CODE_BADDOBBY);
	}
	retVal[len] = '\0';
	return retVal;
}

bwords
bwords_from_units(const char *units)
{
	return bwords_from_string(units, UNIT_SEPARATOR);
}

bwords
bwords_deduplicate(bwords input)
{
	if (input == NULL || input->words == NULL)
		return bwords_new();
	const char **words = input->words;
	size_t num_el = 0;
	size_t cur_el = 0;
	size_t i;
	if (input->num_words < 1) // Can't deduplicate 0 or 1 elements
		return input;
	while (words[num_el] != NULL)
		num_el++;
	qsort(words, num_el, sizeof(char *), qsort_strcmp);
	assert(words[0] != NULL);
	num_el = 0;
	while (words[num_el] != NULL) {
		assert(cur_el <= num_el);
		if (strcmp(words[cur_el], words[num_el]) == 0) {
			num_el++;
			continue;
		}
		assert(cur_el < num_el);
		cur_el++;
		if (cur_el == num_el)
			continue;
		assert(cur_el < num_el);
		for (i = cur_el; i < num_el; i++) {
			if (words[i] == NULL)
				continue;
			free((void*)words[i]);
			words[i] = NULL;
		}
		words[cur_el] = words[num_el];
		words[num_el] = NULL;
		num_el++;
	}
	num_el = 0;
	while (words[num_el] != NULL)
		num_el++;
	input->words = realloc(words, (num_el+1) * sizeof(char *));
	input->words[num_el] = NULL;
	input->words = words;
	input->num_words = num_el;
	input->num_alloc = num_el+1;
	return input;
}

bwords
bwords_from_string(const char *inbuf, const char *separator)
{
	bwords retVal = bwords_new();
	char *word = NULL;
	char *strtok_val = NULL;
	char *buf = NULL;

	/* strdup takes NULL in undefined behaviour, but we don't mind */
	if (inbuf == NULL || separator == NULL)
		goto leave;
	else
		buf = strdup(inbuf);

	if (buf == NULL) {
		perror("Out of memory to make a copy of a string");
		exit(ERR_CODE_NOMEM);
	}
	if (buf[0] == '\0')
		goto leave; /* EARLY RETURN ON NO INPUT AT ALL */
	word = strtok_r(buf, separator, &strtok_val);
	while (word != NULL) {
		retVal = bwords_append_word(retVal, word, 0);
		word = strtok_r(NULL, separator, &strtok_val);
	}
leave:
	if (buf != NULL)
		free(buf);
	return retVal;
}
