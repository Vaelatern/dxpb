#define _POSIX_C_SOURCE 200809L

#include "bxbps.c"

#include "check_main.inc"
#include <check.h>

START_TEST(test_bxbps_get_pkgname)
{
	char *tmp;
	bgraph alls = zhash_new();
	zhash_insert(alls, "foo-bar1_1", &tmp);

	tmp = bxbps_get_pkgname("foo", alls);
	ck_assert_ptr_eq(tmp, NULL);

	tmp = bxbps_get_pkgname("foo-bar1_1", alls);
	ck_assert_str_eq(tmp, "foo-bar1_1");
	free(tmp);

	zhash_insert(alls, "foo", &tmp);

	tmp = bxbps_get_pkgname("foo", alls);
	ck_assert_str_eq(tmp, "foo");
	free(tmp);

	tmp = bxbps_get_pkgname("foo-bar1_1", alls);
	ck_assert_str_eq(tmp, "foo-bar1_1");
	free(tmp);

	zhash_delete(alls, "foo-bar1_1");

	tmp = bxbps_get_pkgname("foo-bar1_1", alls);
	ck_assert_str_eq(tmp, "foo");
	free(tmp);

	tmp = bxbps_get_pkgname("foo>=1.0", alls);
	ck_assert_str_eq(tmp, "foo");
	free(tmp);

	tmp = bxbps_get_pkgname("foo-1.0_1", alls);
	ck_assert_str_eq(tmp, "foo");
	free(tmp);

	tmp = bxbps_get_pkgname("foo<=1.0", alls);
	ck_assert_str_eq(tmp, "foo");
	free(tmp);

	zhash_insert(alls, "python3.4-service_directory", &tmp);

	tmp = bxbps_get_pkgname("python3.4-service_directory>1.5_1", alls);
	ck_assert_str_eq(tmp, "python3.4-service_directory");
	free(tmp);

	zhash_destroy(&alls);

	return;
}
END_TEST

START_TEST(test_bxbps_spec_match)
{
	int rc;

	rc = bxbps_spec_match("vim", "vim", "8.1a4_6");
	ck_assert_int_eq(rc, ERR_CODE_YES);

	rc = bxbps_spec_match("vim-8.1a4_5", "vim", "8.1a4_6");
	ck_assert_int_eq(rc, ERR_CODE_NO);

	rc = bxbps_spec_match("vim-8.1a4_7", "vim", "8.1a4_6");
	ck_assert_int_eq(rc, ERR_CODE_NO);

	rc = bxbps_spec_match("vim-8.1a4_6", "vim", "8.1a4_6");
	ck_assert_int_eq(rc, ERR_CODE_YES);

	rc = bxbps_spec_match("vim", "via", "8.1a4_6");
	ck_assert_int_eq(rc, ERR_CODE_NO);

	rc = bxbps_spec_match("vim", "vima", "8.1a4_6");
	ck_assert_int_eq(rc, ERR_CODE_NO);

	rc = bxbps_spec_match("vima", "vima", "8.1a4_6");
	ck_assert_int_eq(rc, ERR_CODE_YES);

	rc = bxbps_spec_match("vim>8.1_1", "vim", "8.1a4_6");
	ck_assert_int_eq(rc, ERR_CODE_YES);

	rc = bxbps_spec_match("vim>=8.1_1", "vim", "8.1a4_6");
	ck_assert_int_eq(rc, ERR_CODE_YES);

	rc = bxbps_spec_match("vim>=8.1", "vim", "8.1a4_6");
	ck_assert_int_eq(rc, ERR_CODE_YES);

	rc = bxbps_spec_match("vim<=8.2", "vim", "8.1a4_6");
	ck_assert_int_eq(rc, ERR_CODE_YES);

	rc = bxbps_spec_match("vim<=8.2", "vim", "8.2a4_6");
	ck_assert_int_eq(rc, ERR_CODE_NO);
}
END_TEST

Suite * test_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("BXBPS");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_bxbps_get_pkgname);
	tcase_add_test(tc_core, test_bxbps_spec_match);
	suite_add_tcase(s, tc_core);

	return s;
}
