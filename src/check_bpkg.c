#define _POSIX_C_SOURCE 200809L

#include "bpkg.c"
#include "check_main.inc"

START_TEST(test_bpkg_enum_lookup)
{
	for (enum pkg_archs i = ARCH_NOARCH; i < ARCH_NUM_MAX; i++) {
		ck_assert_int_eq(i, bpkg_enum_lookup(pkg_archs_str[i]));
	}
}
END_TEST

START_TEST(test_bpkg_create_allowed_archs)
{
	struct pkg_importer pkgin;
	char *relRay = pkgin.allowed_archs;
	bwords curTest = NULL;
	// Confirm it works if there are no architectures outright allowed
	// This is a way of saying that all are allowed

	curTest = bwords_new();
	bpkg_create_allowed_archs(relRay, curTest);
	for (enum pkg_archs h = ARCH_NOARCH+1; h < ARCH_HOST; h++) {
		ck_assert_int_eq(relRay[h], 1);
	}
	bwords_destroy(&curTest, 0);

	// Confirm it works on any given architecture
	for (enum pkg_archs i = ARCH_NOARCH+1; i < ARCH_HOST; i++) {
		curTest = bwords_new();
		curTest = bwords_append_word(curTest, pkg_archs_str[i], 0);
		bpkg_create_allowed_archs(relRay, curTest);
		for (enum pkg_archs h = ARCH_NOARCH+1; h < ARCH_HOST; h++) {
			if (i == h)
				ck_assert_int_eq(relRay[h], 1);
			else
				ck_assert_int_eq(relRay[h], 0);
		}
		bwords_destroy(&curTest, 0);
	}
	// Confirm it works on any two combined architectures
	for (enum pkg_archs i = ARCH_NOARCH+1; i < ARCH_HOST; i++) {
		for (enum pkg_archs j = ARCH_NOARCH+1; j < ARCH_HOST; j++) {
			if (i == j)
				continue;
			curTest = bwords_new();
			curTest = bwords_append_word(curTest, pkg_archs_str[i], 0);
			curTest = bwords_append_word(curTest, pkg_archs_str[j], 0);
			bpkg_create_allowed_archs(relRay, curTest);
			for (enum pkg_archs h = ARCH_NOARCH+1; h < ARCH_HOST; h++) {
				if (h == i || h == j)
					ck_assert_int_eq(relRay[h], 1);
				else
					ck_assert_int_eq(relRay[h], 0);
			}
			bwords_destroy(&curTest, 0);
		}
	}
}
END_TEST

// I don't know how to cleanly test this. My recommendation is to just use
// valgrind and confirm memory allocated is freed.
START_TEST(test_bpkg_destroy)
{
	struct pkg *tmp = bpkg_init("foo", "bar", pkg_archs_str[ARCH_NOARCH]);
	bpkg_destroy(tmp);
	tmp = NULL;
	bpkg_destroy(tmp);
	tmp = bpkg_init("bar", "baz", pkg_archs_str[ARCH_NOARCH]);
	bpkg_destroy(tmp);
}
END_TEST

START_TEST(test_bpkg_init_basic)
{
	struct pkg *tmp = bpkg_init_basic();
	ck_assert_int_eq(zlist_size(tmp->cross_needs), 0);
	ck_assert_int_eq(zlist_size(tmp->needs), 0);
	ck_assert_int_eq(zlist_size(tmp->needs_me), 0);
	bpkg_destroy(tmp);
}
END_TEST

START_TEST(test_bpkg_init_new)
{
	struct pkg *tmp = bpkg_init_new("foo", "bar");
	ck_assert_str_eq(tmp->name, "foo");
	ck_assert_str_eq(tmp->ver, "bar");
	ck_assert_int_eq(zlist_size(tmp->cross_needs), 0);
	ck_assert_int_eq(zlist_size(tmp->needs), 0);
	ck_assert_int_eq(zlist_size(tmp->needs_me), 0);
	bpkg_destroy(tmp);
}
END_TEST

START_TEST(test_bpkg_clone)
{
	struct pkg *strt = bpkg_init_new("foo", "bar");
	struct pkg *trgt = bpkg_clone(strt);
	ck_assert_str_eq(strt->name, trgt->name);
	ck_assert_str_eq(strt->ver, trgt->ver);
	ck_assert_int_eq(strt->bootstrap, trgt->bootstrap);
	ck_assert_int_eq(strt->can_cross, trgt->can_cross);
	ck_assert_int_eq(strt->broken, trgt->broken);
	ck_assert_int_eq(strt->restricted, trgt->restricted);
	ck_assert_int_eq(strt->arch, trgt->arch);
	bpkg_destroy(trgt);
	ck_assert_str_eq(strt->name, "foo");
	ck_assert_str_eq(strt->ver, "bar");
	bpkg_destroy(strt);

	strt = bpkg_init_new("foo", "bar");
	strt->bootstrap = 1;
	strt->can_cross = 1;
	strt->broken = 0;
	strt->restricted = 1;
	strt->arch = ARCH_TARGET;
	trgt = bpkg_clone(strt);
	ck_assert_str_eq(strt->name, trgt->name);
	ck_assert_str_eq(strt->ver, trgt->ver);
	ck_assert_int_eq(strt->bootstrap, trgt->bootstrap);
	ck_assert_int_eq(strt->can_cross, trgt->can_cross);
	ck_assert_int_eq(strt->broken, trgt->broken);
	ck_assert_int_eq(strt->restricted, trgt->restricted);
	ck_assert_int_eq(strt->arch, trgt->arch);
	bpkg_destroy(trgt);
	bpkg_destroy(strt);
}
END_TEST

Suite * test_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("BPKG");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_bpkg_enum_lookup);
	tcase_add_test(tc_core, test_bpkg_create_allowed_archs);
	tcase_add_test(tc_core, test_bpkg_destroy);
	tcase_add_test(tc_core, test_bpkg_init_basic);
	tcase_add_test(tc_core, test_bpkg_init_new);
	tcase_add_test(tc_core, test_bpkg_clone);
	suite_add_tcase(s, tc_core);

	return s;
}
