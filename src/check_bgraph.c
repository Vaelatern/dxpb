#include "bgraph.c"
#include "check_main.c"

START_TEST(test_basic_bgraph)
{
	bgraph grph;
	zlist_t *list;
	grph = bgraph_new();
	ck_assert_ptr_nonnull(grph);
	ck_assert_int_eq(zhash_size(grph), ARCH_NUM_MAX);
	for (zhash_t *item = zhash_first(grph); item != NULL;
					item = zhash_next(grph)) {
		ck_assert_int_eq(zhash_size(item), 0);
	}
	list = bgraph_what_next_for_arch(grph, ARCH_NOARCH);
	ck_assert_ptr_nonnull(list);
	ck_assert_int_eq(zlist_size(list), 0);
	zlist_destroy(&list);
	bgraph_destroy(&grph);
	ck_assert_ptr_null(grph);
}
END_TEST

START_TEST(test_basic_bgraph_with_pkg)
{
	int rc;
	bgraph grph;
	zlist_t *list;
	struct pkg *newpkg = bpkg_init("foo", "bar", "noarch");
	grph = bgraph_new();
	ck_assert_ptr_nonnull(grph);
	rc = bgraph_insert_pkg(grph, newpkg);
	ck_assert_int_eq(rc, ERR_CODE_OK);
	list = bgraph_what_next_for_arch(grph, ARCH_NOARCH);
	ck_assert_ptr_nonnull(list);
	ck_assert_int_eq(zlist_size(list), 1);
	ck_assert_ptr_eq(newpkg, zlist_first(list));
	zlist_destroy(&list);
	bgraph_destroy(&grph);
	ck_assert_ptr_null(grph);
}
END_TEST

START_TEST(test_basic_bgraph_with_pkg_replacement)
{
	int rc;
	bgraph grph;
	zlist_t *list;
	struct pkg *newpkg = bpkg_init("foo", "bar", "noarch");
	struct pkg *reppkg = bpkg_init("foo", "baz", "noarch");

	grph = bgraph_new();
	ck_assert_ptr_nonnull(grph);

	rc = bgraph_insert_pkg(grph, newpkg);
	ck_assert_int_eq(rc, ERR_CODE_OK);

	list = bgraph_what_next_for_arch(grph, ARCH_NOARCH);
	ck_assert_ptr_nonnull(list);
	ck_assert_int_eq(zlist_size(list), 1);
	ck_assert_ptr_eq(newpkg, zlist_first(list));
	zlist_destroy(&list);

	rc = bgraph_insert_pkg(grph, reppkg);
	ck_assert_int_eq(rc, ERR_CODE_OK);
	list = bgraph_what_next_for_arch(grph, ARCH_NOARCH);
	ck_assert_ptr_nonnull(list);
	ck_assert_int_eq(zlist_size(list), 1);
	ck_assert_ptr_eq(reppkg, zlist_first(list));
	zlist_destroy(&list);

	bgraph_destroy(&grph);
	ck_assert_ptr_null(grph);
}
END_TEST

Suite * test_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("BGRAPH");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_basic_bgraph);
	tcase_add_test(tc_core, test_basic_bgraph_with_pkg);
	tcase_add_test(tc_core, test_basic_bgraph_with_pkg_replacement);
	suite_add_tcase(s, tc_core);

	return s;
}
