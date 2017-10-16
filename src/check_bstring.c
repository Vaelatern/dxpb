#define _POSIX_C_SOURCE 200809L

#include "bstring.c"
#include "check_main.inc"

START_TEST(test_bstring_add)
{
	char *tmp = NULL;
	uint32_t alloced, len;
	alloced = len = 0;

	tmp = bstring_add(tmp, "Hello", &alloced, &len);
	ck_assert_str_eq(tmp, "Hello");
	tmp = bstring_add(tmp, " World", &alloced, &len);
	ck_assert_str_eq(tmp, "Hello World");
	tmp = bstring_add(tmp, "!", &alloced, &len);
	ck_assert_str_eq(tmp, "Hello World!");
	tmp = bstring_add(tmp, "", &alloced, &len);
	ck_assert_str_eq(tmp, "Hello World!");
	tmp = bstring_add(tmp, NULL, &alloced, &len);
	ck_assert_str_eq(tmp, "Hello World!");
	tmp = bstring_add(tmp, " How are you?", NULL, NULL);
	ck_assert_str_eq(tmp, "Hello World! How are you?");
	free(tmp);

	tmp = NULL;
	tmp = bstring_add(tmp, "How are you?", NULL, NULL);
	ck_assert_str_eq(tmp, "How are you?");
	free(tmp);
}
END_TEST

Suite * test_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("BSTRING");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_bstring_add);
	suite_add_tcase(s, tc_core);

	return s;
}
