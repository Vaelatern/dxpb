#define _POSIX_C_SOURCE 200809L

#include "bworker.c"
#include "check_main.inc"

START_TEST(test_bworker_basic)
{
	struct bworkgroup *grp = bworker_group_new();
	bworker_group_destroy(&grp);
}
END_TEST

START_TEST(test_bworker_subs)
{
	struct bworkgroup *grp = bworker_group_new();
	struct bworksubgroup *g1 = bworker_subgroup_new(grp);
	struct bworksubgroup *g2 = bworker_subgroup_new(grp);
	struct bworksubgroup *g3 = bworker_subgroup_new(grp);
	struct bworksubgroup *g4 = bworker_subgroup_new(grp);
	struct bworksubgroup *g5 = bworker_subgroup_new(grp);
	bworker_subgroup_destroy(&g5);
	bworker_subgroup_destroy(&g4);
	bworker_subgroup_destroy(&g2);
	bworker_subgroup_destroy(&g1);
	bworker_subgroup_destroy(&g3);
	bworker_group_destroy(&grp);
}
END_TEST

START_TEST(test_bworker_basic_workers)
{
	struct bworkgroup *grp = bworker_group_new();
	bworker_group_insert(grp, 3, 0, ARCH_ARMV7L_MUSL, ARCH_X86_64, 1, 100);
	bworker_group_insert(grp, 2, 0, ARCH_ARMV7L_MUSL, ARCH_X86_64, 1, 100);
	bworker_group_insert(grp, 1, 0, ARCH_ARMV7L, ARCH_X86_64, 1, 100);
	bworker_group_insert(grp, 0, 0, ARCH_ARMV7L, ARCH_X86_64, 1, 100);
	for (int i = 0; i < 4; i++) {
		struct bworker *wrkr = bworker_from_remote_addr(grp, i, 0);
		bworker_group_remove(wrkr);
		ck_assert_int_eq(wrkr->addr, UINT16_MAX);
	}
	bworker_group_destroy(&grp);
}
END_TEST

START_TEST(test_bworker_subs_workers)
{
	struct bworkgroup *grp = bworker_group_new();
	struct bworksubgroup *g1 = bworker_subgroup_new(grp);
	struct bworksubgroup *g2 = bworker_subgroup_new(grp);
	struct bworksubgroup *g3 = bworker_subgroup_new(grp);
	struct bworksubgroup *g4 = bworker_subgroup_new(grp);
	struct bworksubgroup *g5 = bworker_subgroup_new(grp);
	bworker_subgroup_insert(g1, 0, 0, ARCH_ARMV7L_MUSL, ARCH_X86_64, 1, 100);
	struct bworker *w1 = bworker_from_sub_remote_addr(g1, 0, 0);
	ck_assert_ptr_nonnull(w1);
	bworker_subgroup_insert(g2, 0, 0, ARCH_ARMV7L_MUSL, ARCH_X86_64, 1, 100);
	struct bworker *w2 = bworker_from_sub_remote_addr(g2, 0, 0);
	ck_assert_ptr_nonnull(w2);
	bworker_subgroup_insert(g3, 0, 0, ARCH_ARMV7L, ARCH_X86_64, 1, 100);
	struct bworker *w3 = bworker_from_sub_remote_addr(g3, 0, 0);
	ck_assert_ptr_nonnull(w3);
	bworker_subgroup_insert(g4, 0, 0, ARCH_X86_64, ARCH_X86_64, 0, 100);
	struct bworker *w4 = bworker_from_sub_remote_addr(g4, 0, 0);
	ck_assert_ptr_nonnull(w4);
	bworker_subgroup_insert(g5, 0, 0, ARCH_X86_64, ARCH_X86_64, 0, 100);
	struct bworker *w5 = bworker_from_sub_remote_addr(g5, 0, 0);
	ck_assert_ptr_nonnull(w5);
	struct bworker *w5a = bworker_from_sub_remote_addr(g5, 1, 0);
	ck_assert_ptr_null(w5a);
	bworker_subgroup_insert(g5, 1, 0, ARCH_X86_64, ARCH_X86_64, 0, 100);
	struct bworker *w5b = bworker_from_sub_remote_addr(g5, 1, 0);
	ck_assert_ptr_nonnull(w5b);
	bworker_subgroup_destroy(&g5);
	bworker_subgroup_destroy(&g1);
	bworker_subgroup_destroy(&g3);
	bworker_subgroup_destroy(&g2);
	bworker_subgroup_destroy(&g4);
	bworker_group_destroy(&grp);
}
END_TEST

Suite * test_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("BWORKER");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_bworker_basic);
	tcase_add_test(tc_core, test_bworker_subs);
	tcase_add_test(tc_core, test_bworker_basic_workers);
	tcase_add_test(tc_core, test_bworker_subs_workers);
	suite_add_tcase(s, tc_core);

	return s;
}
