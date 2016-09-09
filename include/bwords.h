/*
 * bwords.h
 *
 * Module for dealing with char ** lists of words
 */

#ifdef DXPB_BWORDS_H
#error "Clean your dependencies"
#endif
#define DXPB_BWORDS_H

struct bwords {
	const char **words;
	size_t num_words;
	size_t num_alloc;
};

typedef struct bwords * bwords;

bwords	 bwords_new(void);
void	 bwords_destroy(bwords *, char);
bwords	 bwords_append_word(bwords, const char *, char);
bwords	 bwords_merge_words(bwords, bwords);
char    *bwords_to_units(bwords);
bwords	 bwords_from_units(const char *);
bwords	 bwords_deduplicate(bwords);
bwords	 bwords_from_string(const char *, const char *);
