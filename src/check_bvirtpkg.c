#define _POSIX_C_SOURCE 200809L

#include "bvirtpkg.c"
#include "check_main.inc"

START_TEST(test_bvirtpkg_read)
{
}
END_TEST

Suite * test_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("BVIRTPKG");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_bvirtpkg_read);
	suite_add_tcase(s, tc_core);

	return s;
}
