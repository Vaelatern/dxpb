#define _POSIX_C_SOURCE 200809L

#include "bdb.c"
#include "check_main.inc"

START_TEST(test_bdb_basic)
{
	struct bdb_bound_params *params;
	params = bdb_params_init(":memory:");
	bdb_params_destroy(&params);
}
END_TEST

START_TEST(test_bdb_basic_hash)
{
	const char *hash = "6c5809f60a75bf6f9544ce4f76b112d330863082";
	struct bdb_bound_params *params;
	params = bdb_params_init(":memory:");
	bdb_write_hash_to_db(hash, params);
	ck_assert_str_eq(hash, bdb_read_hash(params));
	bdb_params_destroy(&params);
}
END_TEST

void // stolen from check_bgraph.c
do_pkg_needs(struct pkg *pkg, bwords *host, bwords *target)
{
	if (target) {
		pkg->wneeds_cross_target = bwords_clone(*target);
		pkg->wneeds_native_target = bwords_clone(*target);
	} else {
		pkg->wneeds_cross_target = bwords_new();
		pkg->wneeds_native_target = bwords_new();
	}
	if (host) {
		pkg->wneeds_cross_host = bwords_clone(*host);
		pkg->wneeds_native_host = bwords_clone(*host);
	} else {
		pkg->wneeds_cross_host = bwords_new();
		pkg->wneeds_native_host = bwords_new();
	}
	assert(pkg->wneeds_cross_target != NULL);
	assert(pkg->wneeds_native_target != NULL);
	assert(pkg->wneeds_cross_host != NULL);
	assert(pkg->wneeds_native_host != NULL);
}

void
new_package(bgraph grph, char *name, char *ver, int noarch, int present,
		bwords *host, bwords *target)
{
	struct pkg *pkg;
	int rc;
	if (noarch) {
		pkg = bpkg_init(name, ver, pkg_archs_str[ARCH_NOARCH]);
		do_pkg_needs(pkg, host, target);
		rc = bgraph_insert_pkg(grph, pkg);
		ck_assert_int_eq(rc, ERR_CODE_OK);
		if (!present)
			bgraph_mark_pkg_not_in_progress(grph, name, ver, ARCH_NOARCH);
		else
			bgraph_mark_pkg_present(grph, name, ver, ARCH_NOARCH);
	} else {
		for (enum pkg_archs i = 1; i <= ARCH_HOST; i++) {
			if (i == ARCH_HOST)
				i = ARCH_TARGET;
			pkg = bpkg_init(name, ver, pkg_archs_str[i]);
			do_pkg_needs(pkg, host, target);
			rc = bgraph_insert_pkg(grph, pkg);
			ck_assert_int_eq(rc, ERR_CODE_OK);
			if (!present)
				bgraph_mark_pkg_not_in_progress(grph, name, ver, i);
			else
				bgraph_mark_pkg_present(grph, name, ver, i);
		}
	}
	bwords_destroy(host, 0);
	bwords_destroy(target, 0);
}

START_TEST(test_bdb_basic_tree)
{
	bgraph a = NULL, b = NULL;
	a = bgraph_new();
	ck_assert_ptr_nonnull(a);
	int rc;
	struct bdb_bound_params *params;
	params = bdb_params_init(":memory:");

	new_package(a, "foo", "0.0.1", 0, 0, NULL, NULL);
	bdb_write_tree(a, params);
	rc = bdb_read_pkgs_to_graph(&b, params);
	bgraph_destroy(&b);
	bgraph_destroy(&a);

	bdb_params_destroy(&params);
}
END_TEST

START_TEST(test_bdb_basic_both)
{
	const char *hash = "6c5809f60a75bf6f9544ce4f76b112d330863082";
	bgraph a = NULL;
	a = bgraph_new();
	ck_assert_ptr_nonnull(a);

	new_package(a, "foo", "0.0.1", 0, 0, NULL, NULL);
	bdb_write_all(":memory:", a, hash);

	bgraph_destroy(&a);
}
END_TEST

Suite * test_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("BDB");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_bdb_basic);
	tcase_add_test(tc_core, test_bdb_basic_hash);
	tcase_add_test(tc_core, test_bdb_basic_tree);
	tcase_add_test(tc_core, test_bdb_basic_both);
	suite_add_tcase(s, tc_core);

	return s;
}
