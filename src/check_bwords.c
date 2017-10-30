#define _POSIX_C_SOURCE 200809L

#include "bwords.c"
#include "check_main.inc"

START_TEST(test_bwords_add_word)
{
	bwords subj = bwords_new();
	ck_assert_ptr_null(subj->words[0]);
	ck_assert_int_eq(subj->num_words, 0);
	ck_assert_int_eq(subj->num_alloc, 1);
	subj = bwords_append_word(subj, "Hello", 0);
	ck_assert_str_eq(subj->words[0], "Hello");
	ck_assert_int_eq(subj->num_words, 1);
	ck_assert_int_ge(subj->num_alloc, 2);
	ck_assert_ptr_null(subj->words[1]);
	subj = bwords_append_word(subj, NULL, 0);
	ck_assert_int_eq(subj->num_words, 1);
	ck_assert_int_ge(subj->num_alloc, 2);
	ck_assert_str_eq(subj->words[0], "Hello");
	ck_assert_ptr_null(subj->words[1]);
	subj = bwords_append_word(subj, "World", 0);
	ck_assert_int_eq(subj->num_words, 2);
	ck_assert_int_ge(subj->num_alloc, 2);
	ck_assert_str_eq(subj->words[0], "Hello");
	ck_assert_str_eq(subj->words[1], "World");
	ck_assert_ptr_null(subj->words[2]);
	bwords_destroy(&subj, 1);
}
END_TEST

START_TEST(test_bwords_have_sane_defaults)
{
	bwords subj = NULL;
	char *tmp = NULL;

	subj = bwords_new();
	ck_assert_ptr_nonnull(subj);
	ck_assert_ptr_nonnull(subj->words);
	ck_assert_ptr_null(subj->words[0]);
	ck_assert_int_eq(subj->num_alloc, 1);
	ck_assert_int_eq(subj->num_words, 0);
	bwords_destroy(&subj, 1);
	ck_assert_ptr_null(subj);

	subj = bwords_append_word(NULL, NULL, 0);
	ck_assert_ptr_nonnull(subj);
	ck_assert_ptr_nonnull(subj->words);
	ck_assert_ptr_null(subj->words[0]);
	ck_assert_int_eq(subj->num_alloc, 1);
	ck_assert_int_eq(subj->num_words, 0);
	bwords_destroy(&subj, 1);
	ck_assert_ptr_null(subj);

	subj = bwords_merge_words(NULL, NULL);
	ck_assert_ptr_nonnull(subj);
	ck_assert_ptr_nonnull(subj->words);
	ck_assert_ptr_null(subj->words[0]);
	ck_assert_int_eq(subj->num_alloc, 1);
	ck_assert_int_eq(subj->num_words, 0);
	bwords_destroy(&subj, 1);
	ck_assert_ptr_null(subj);

	subj = bwords_from_string(NULL, NULL);
	ck_assert_ptr_nonnull(subj);
	ck_assert_ptr_nonnull(subj->words);
	ck_assert_ptr_null(subj->words[0]);
	ck_assert_int_eq(subj->num_alloc, 1);
	ck_assert_int_eq(subj->num_words, 0);
	bwords_destroy(&subj, 1);
	ck_assert_ptr_null(subj);

	subj = bwords_from_units(NULL);
	ck_assert_ptr_nonnull(subj);
	ck_assert_ptr_nonnull(subj->words);
	ck_assert_ptr_null(subj->words[0]);
	ck_assert_int_eq(subj->num_alloc, 1);
	ck_assert_int_eq(subj->num_words, 0);
	bwords_destroy(&subj, 1);
	ck_assert_ptr_null(subj);

	subj = bwords_deduplicate(NULL);
	ck_assert_ptr_nonnull(subj);
	ck_assert_ptr_nonnull(subj->words);
	ck_assert_ptr_null(subj->words[0]);
	ck_assert_int_eq(subj->num_alloc, 1);
	ck_assert_int_eq(subj->num_words, 0);
	bwords_destroy(&subj, 1);
	ck_assert_ptr_null(subj);

	tmp = bwords_to_units(NULL);
	ck_assert_ptr_nonnull(tmp);
	ck_assert_str_eq(tmp, "");
	free(tmp);
	tmp = NULL;
}
END_TEST

START_TEST(test_bwords_merge_words)
{
	bwords first = NULL;
	bwords second = NULL;
	bwords third = NULL;
	first = bwords_append_word(first, "Hello", 0);
	ck_assert_ptr_nonnull(first);
	first = bwords_append_word(first, "World", 0);
	ck_assert_str_eq(first->words[0], "Hello");
	ck_assert_str_eq(first->words[1], "World");
	ck_assert_ptr_null(first->words[2]);
	first = bwords_merge_words(first, NULL);
	ck_assert_str_eq(first->words[0], "Hello");
	ck_assert_str_eq(first->words[1], "World");
	ck_assert_ptr_null(first->words[2]);
	first = bwords_merge_words(NULL, first);
	ck_assert_str_eq(first->words[0], "Hello");
	ck_assert_str_eq(first->words[1], "World");
	ck_assert_ptr_null(first->words[2]);
	third = bwords_append_word(NULL, "What", 0);
	ck_assert_str_eq(third->words[0], "What");
	ck_assert_ptr_null(third->words[1]);
	third = bwords_merge_words(first, third);
	ck_assert_str_eq(third->words[0], "Hello");
	ck_assert_str_eq(third->words[1], "World");
	ck_assert_str_eq(third->words[2], "What");
	ck_assert_ptr_null(third->words[3]);
	bwords_destroy(&first, 0);
	first = bwords_append_word(first, "Hello", 0);
	first = bwords_append_word(first, "World", 0);
	ck_assert_str_eq(first->words[0], "Hello");
	ck_assert_str_eq(first->words[1], "World");
	ck_assert_ptr_null(first->words[2]);
	second = bwords_append_word(second, "What", 0);
	second = bwords_append_word(second, "Is", 0);
	second = bwords_append_word(second, "Up", 0);
	ck_assert_ptr_nonnull(second);
	ck_assert_ptr_nonnull(second->words);
	ck_assert_str_eq(second->words[0], "What");
	ck_assert_str_eq(second->words[1], "Is");
	ck_assert_str_eq(second->words[2], "Up");
	ck_assert_ptr_null(second->words[3]);
	first = bwords_merge_words(first, second);
	ck_assert_str_eq(first->words[0], "Hello");
	ck_assert_str_eq(first->words[1], "World");
	ck_assert_str_eq(first->words[2], "What");
	ck_assert_str_eq(first->words[3], "Is");
	ck_assert_str_eq(first->words[4], "Up");
	ck_assert_ptr_null(first->words[5]);
	bwords_destroy(&first, 0);
}
END_TEST

START_TEST(test_bwords_to_units)
{
	bwords words = bwords_new();
	char *tmp = NULL;
	words = bwords_append_word(words, "Hello", 0);
	tmp = bwords_to_units(words);
	ck_assert_str_eq(tmp, "Hello");
	free(tmp);
	words = bwords_append_word(words, "World", 0);
	tmp = bwords_to_units(words);
	ck_assert_str_eq(tmp, "Hello\037World");
	free(tmp);
	words = bwords_append_word(words, "Wassup", 0);
	tmp = bwords_to_units(words);
	ck_assert_str_eq(tmp, "Hello\037World\037Wassup");
	free(tmp);
	bwords_destroy(&words, 1);
	words = NULL;
	words = bwords_append_word(words, "Hello", 0);
	words = bwords_append_word(words, "Hello?,.", 0);
	words = bwords_append_word(words, "Hello", 0);
	words = bwords_append_word(words, "Hi", 0);
	tmp = bwords_to_units(words);
	ck_assert_str_eq(tmp, "Hello\037Hello?,.\037Hello\037Hi");
	free(tmp);
	bwords_destroy(&words, 1);
	tmp = bwords_to_units(NULL);
	ck_assert_str_eq(tmp, "");
	free(tmp);
	words = NULL;
	words = bwords_append_word(words, NULL, 0);
	tmp = bwords_to_units(words);
	ck_assert_str_eq(tmp, "");
	free(tmp);
	free(words);
}
END_TEST

START_TEST(test_bwords_from_units)
{
	bwords words;
	char *tmp = NULL;
	char *tmp1 = "Hello";
	char *tmp2 = "Hello\037World!";
	char *tmp3 = "Hello\037World!\037Wassup";
	char *tmp4 = "Hello\037Hello?,.\037Hello\037Hi";
	char *tmp5 = "";

	words = NULL;
	words = bwords_append_word(words, NULL, 0);
	tmp = bwords_to_units(words);
	ck_assert_str_eq(tmp, "");
	bwords_destroy(&words, 1);
	words = bwords_from_units(tmp);
	ck_assert_ptr_nonnull(words);
	ck_assert_ptr_nonnull(words->words);
	ck_assert_ptr_null(words->words[0]);
	free(tmp);
	bwords_destroy(&words, 1);

	words = bwords_from_units(tmp1);
	ck_assert_ptr_nonnull(words);
	ck_assert_ptr_nonnull(words->words);
	ck_assert_ptr_nonnull(words->words[0]);
	ck_assert_ptr_null(words->words[1]);
	ck_assert_str_eq(words->words[0], "Hello");
	bwords_destroy(&words, 1);

	words = bwords_from_units(tmp2);
	ck_assert_ptr_nonnull(words);
	ck_assert_ptr_nonnull(words->words);
	ck_assert_ptr_nonnull(words->words[0]);
	ck_assert_ptr_nonnull(words->words[1]);
	ck_assert_ptr_null(words->words[2]);
	ck_assert_str_eq(words->words[0], "Hello");
	ck_assert_str_eq(words->words[1], "World!");
	bwords_destroy(&words, 1);

	words = bwords_from_units(tmp3);
	ck_assert_ptr_nonnull(words);
	ck_assert_ptr_nonnull(words->words);
	ck_assert_ptr_nonnull(words->words[0]);
	ck_assert_ptr_nonnull(words->words[1]);
	ck_assert_ptr_nonnull(words->words[2]);
	ck_assert_ptr_null(words->words[3]);
	ck_assert_str_eq(words->words[0], "Hello");
	ck_assert_str_eq(words->words[1], "World!");
	ck_assert_str_eq(words->words[2], "Wassup");
	bwords_destroy(&words, 1);

	words = bwords_from_units(tmp4);
	ck_assert_ptr_nonnull(words);
	ck_assert_ptr_nonnull(words->words);
	ck_assert_ptr_nonnull(words->words[0]);
	ck_assert_ptr_nonnull(words->words[1]);
	ck_assert_ptr_nonnull(words->words[2]);
	ck_assert_ptr_nonnull(words->words[3]);
	ck_assert_ptr_null(words->words[4]);
	ck_assert_str_eq(words->words[0], "Hello");
	ck_assert_str_eq(words->words[1], "Hello?,.");
	ck_assert_str_eq(words->words[2], "Hello");
	ck_assert_str_eq(words->words[3], "Hi");
	bwords_destroy(&words, 1);

	words = bwords_from_units(tmp5);
	ck_assert_ptr_nonnull(words->words);
	ck_assert_ptr_null(words->words[0]);
	bwords_destroy(&words, 0);
}
END_TEST

START_TEST(test_deduplicate_words)
{
	bwords words = bwords_new();
	ck_assert_ptr_nonnull(words);
	words = bwords_append_word(words, "Hello", 0);
	words = bwords_append_word(words, "Hello", 0);
	words = bwords_append_word(words, "What a world", 0);
	words = bwords_append_word(words, "Hello", 0);
	words = bwords_deduplicate(words);
	ck_assert_ptr_nonnull(words->words);
	ck_assert_ptr_nonnull(words->words[0]);
	ck_assert_ptr_nonnull(words->words[1]);
	ck_assert( (!strcmp("Hello", words->words[0]) &&
				(!strcmp("What a world", words->words[1])))
		|| (!strcmp("Hello", words->words[1]) &&
				  (!strcmp("What a world", words->words[0]))));
	ck_assert_ptr_null(words->words[2]);
	bwords_destroy(&words, 1);
}
END_TEST

Suite * test_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("BWORDS");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_bwords_have_sane_defaults);
	tcase_add_test(tc_core, test_bwords_add_word);
	tcase_add_test(tc_core, test_bwords_merge_words);
	tcase_add_test(tc_core, test_bwords_to_units);
	tcase_add_test(tc_core, test_bwords_from_units);
	tcase_add_test(tc_core, test_deduplicate_words);
	suite_add_tcase(s, tc_core);

	return s;
}
