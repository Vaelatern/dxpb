#error "This class must be re-worked before it can be relied upon as a test of bworker"

#include "bworker.c"
#include "check_main.c"

#define GIMME_A_SUBGROUP struct bworkgroup *group = NULL; struct bworksubgroup *subgroup = NULL; group = bworker_group_new(); ck_assert_ptr_nonnull(group); ck_assert_int_eq(group->num_workers, 0); ck_assert_int_eq(group->direct_use, 1); subgroup = bworker_subgroup_init(group); ck_assert_ptr_nonnull(subgroup); ck_assert_int_eq(group->direct_use, 0);

#define GIMME_A_GROUP struct bworkgroup *group = NULL; group = bworker_group_new(); ck_assert_ptr_nonnull(group); ck_assert_int_eq(group->num_workers, 0); ck_assert_int_eq(group->direct_use, 1);

#define DESTROY_MY_SUBGROUP bworker_subgroup_destroy(&subgroup); ck_assert_ptr_null(subgroup); bworker_group_destroy(&group); ck_assert_ptr_null(group);

#define DESTROY_MY_GROUP bworker_group_destroy(&group); ck_assert_ptr_null(group);


START_TEST(test_bworker_basic)
{
	struct bworkgroup *group = NULL;
	size_t rc;
	uint16_t addr;

	group = bworker_group_new();
	ck_assert_ptr_nonnull(group);
	ck_assert_int_eq(group->num_workers, 0);
	ck_assert_int_eq(group->direct_use, 1);
	for (size_t i = 0; i < BWORKER_NUM_MAX; i++) {
		ck_assert_int_eq((group->workers[i]).arch, ARCH_NUM_MAX);
		ck_assert_int_eq((group->workers[i]).hostarch, ARCH_NUM_MAX);
		ck_assert_int_eq((group->workers[i]).addr, BWORKER_NUM_MAX + 1);
	}

	for (size_t i = 0; i < BWORKER_NUM_MAX - 19; i++) {
		addr = i + 19;
		rc = bworker_group_insert(group, addr, ARCH_NOARCH, ARCH_X86_64, 0, 0);
		ck_assert_int_eq(rc, i);
		ck_assert_int_eq(group->num_workers, i+1);
		ck_assert_int_eq(group->direct_use, 1);
	}
	for (size_t i = BWORKER_NUM_MAX - 19; i < BWORKER_NUM_MAX; i++) {
		addr = i - (BWORKER_NUM_MAX - 19);
		rc = bworker_group_insert(group, addr, ARCH_NOARCH, ARCH_X86_64, 0, 0);
		ck_assert_int_eq(rc, i);
		ck_assert_int_eq(group->num_workers, i+1);
		ck_assert_int_eq(group->direct_use, 1);
	}

	ck_assert_int_eq(group->num_workers, BWORKER_NUM_MAX);
	rc = bworker_group_insert(group, BWORKER_NUM_MAX, ARCH_NOARCH, ARCH_X86_64, 0, 0);
	ck_assert_int_eq(rc, BWORKER_NUM_MAX+1);
	ck_assert_int_eq(group->num_workers, BWORKER_NUM_MAX);
	ck_assert_int_eq(group->direct_use, 1);

	for (size_t i = 0; i < BWORKER_NUM_MAX - 19; i++) {
		addr = i + 19;
		rc = bworker_group_remove(group, addr);
		ck_assert_int_eq(rc, i);
		ck_assert_int_eq(group->num_workers, BWORKER_NUM_MAX-i-1);
		ck_assert_int_eq(group->direct_use, 1);
	}
	for (size_t i = BWORKER_NUM_MAX - 19; i < BWORKER_NUM_MAX; i++) {
		addr = i - (BWORKER_NUM_MAX - 19);
		rc = bworker_group_remove(group, addr);
		ck_assert_int_eq(rc, i);
		ck_assert_int_eq(group->num_workers, BWORKER_NUM_MAX-i-1);
		ck_assert_int_eq(group->direct_use, 1);
	}
	ck_assert_int_eq(group->num_workers, 0);
	rc = bworker_group_remove(group, 0);
	ck_assert_int_eq(rc, BWORKER_NUM_MAX);
	ck_assert_int_eq(group->num_workers, 0);
	ck_assert_int_eq(group->direct_use, 1);

	bworker_group_destroy(&group);
	ck_assert_ptr_null(group);
}
END_TEST

START_TEST(test_bworker_subgroup_assign_slot)
{
	struct bworkgroup *group = NULL;
	struct bworksubgroup *subgroup = NULL;
	struct bworksubgroup *newsubgroup = NULL;
	int rc;
	group = bworker_group_new();
	ck_assert_ptr_nonnull(group);

	subgroup = bworker_subgroup_alloc(group);
	ck_assert_int_eq(group->direct_use, 0);
	rc = bworker_subgroup_assign_slot(group, subgroup);
	ck_assert_int_eq(rc, 1);
	ck_assert_int_lt(subgroup->min, subgroup->max);
	{
		int matches = 0;
		for (int i = 0; i < BWORKER_NUM_MAX/BWORKER_SUBGRP_SIZE; i++) {
			if (group->slots[i] != NULL) {
				matches++;
				ck_assert_ptr_eq(group->slots[i], subgroup);
			}
		}
		ck_assert_int_eq(matches, 1);
	}
	newsubgroup = bworker_subgroup_alloc(group);
	rc = bworker_subgroup_assign_slot(group, newsubgroup);
	ck_assert_int_eq(rc, 1);
	ck_assert_int_lt(newsubgroup->min, newsubgroup->max);
	{
		int matches = 0;
		for (int i = 0; i < BWORKER_NUM_MAX/BWORKER_SUBGRP_SIZE; i++) {
			if (group->slots[i] != NULL) {
				matches++;
				if (group->slots[i] != subgroup)
					ck_assert_ptr_eq(group->slots[i], newsubgroup);
			}
		}
		ck_assert_int_eq(matches, 2);
	}

	bworker_group_destroy(&group);
	ck_assert_ptr_null(group);
}
END_TEST

START_TEST(test_bworker_subgroups)
{
	struct bworkgroup *group = NULL;
	struct bworksubgroup *subgroup = NULL;
	struct bworksubgroup *newsubgroup = NULL;
	int rc;

	group = bworker_group_new();
	ck_assert_ptr_nonnull(group);
	ck_assert_int_eq(group->num_workers, 0);
	ck_assert_int_eq(group->direct_use, 1);
	for (size_t i = 0; i < BWORKER_NUM_MAX; i++) {
		ck_assert_int_eq((group->workers[i]).arch, ARCH_NUM_MAX);
		ck_assert_int_eq((group->workers[i]).hostarch, ARCH_NUM_MAX);
		ck_assert_int_eq((group->workers[i]).addr, BWORKER_NUM_MAX + 1);
	}

	// These next two functions are static.
	// Next we will test a function that composes them both.
	subgroup = bworker_subgroup_alloc(group);
	ck_assert_int_eq(group->direct_use, 0);
	rc = bworker_subgroup_assign_slot(group, subgroup);
	ck_assert_int_eq(rc, 1);
	{
		int matches = 0;
		for (int i = 0; i < BWORKER_NUM_MAX/BWORKER_SUBGRP_SIZE; i++) {
			if (group->slots[i] != NULL) {
				matches++;
				ck_assert_ptr_eq(group->slots[i], subgroup);
			}
		}
		ck_assert_int_eq(matches, 1);
	}

	// This function is more usable.
	newsubgroup = bworker_subgroup_init(group);
	ck_assert_ptr_nonnull(newsubgroup);
	ck_assert_int_eq(group->direct_use, 0);
	{
		int matches = 0;
		for (int i = 0; i < BWORKER_NUM_MAX/BWORKER_SUBGRP_SIZE; i++) {
			if (group->slots[i] != NULL) {
				matches++;
				if (group->slots[i] != subgroup)
					ck_assert_ptr_eq(group->slots[i], newsubgroup);
			}
		}
		ck_assert_int_eq(matches, 2);
	}
	subgroup = newsubgroup; // we will want this later.

	// But returns NULL if it can't allocate a slot
	for (size_t i = 2; i < BWORKER_NUM_MAX/BWORKER_SUBGRP_SIZE; i++) {
		newsubgroup = bworker_subgroup_init(group);
		ck_assert_ptr_nonnull(newsubgroup);
	}
	newsubgroup = bworker_subgroup_init(group);
	ck_assert_ptr_null(newsubgroup);

	// And of course connections can go away
	// This is most tricky because of the jobs that are represented... but
	// actually we don't need to worry about them. The appropiate way to
	// handle them is garbage collection separately for any workers
	// inadequately cleaned.
	ck_assert_ptr_nonnull(subgroup);
	rc = bworker_subgroup_destroy(&subgroup);
	ck_assert_ptr_null(subgroup);

	// And on a member with no real group?
	subgroup = bworker_subgroup_alloc(group);
	rc = bworker_subgroup_destroy(&subgroup);
	ck_assert_ptr_nonnull(subgroup);
	rc = bworker_subgroup_assign_slot(group, subgroup);
	rc = bworker_subgroup_destroy(&subgroup);
	ck_assert_ptr_null(subgroup);


	bworker_group_destroy(&group);
	ck_assert_ptr_null(group);
}
END_TEST

START_TEST(test_bworker_subgroup_insert)
{
	GIMME_A_SUBGROUP;
	int rc, lastrc = -1;

	// Now for the real test

	for (int i = 0; i < BWORKER_SUBGRP_SIZE; i++) {
		rc = bworker_subgroup_insert(subgroup, 0, i % ARCH_HOST,
				(2*i) % ARCH_HOST, i % 2, 2*i);
		ck_assert_int_gt(rc, lastrc);
		lastrc = rc;
	}
	for (int i = 0; i < BWORKER_SUBGRP_SIZE; i++) {
		rc = bworker_subgroup_insert(subgroup, 1, ARCH_NOARCH,
				ARCH_X86_64, 0, 0);
		ck_assert_int_eq(rc, BWORKER_NUM_MAX + 1);
	}
	// ensure only the first loop assigned workers
	for (int i = 0; i < BWORKER_SUBGRP_SIZE; i++) {
		struct bworker *wrk = &(subgroup->grp->workers[subgroup->min+i]);
		ck_assert_int_eq(wrk->addr, 0);
		ck_assert_int_eq(wrk->arch, i%ARCH_HOST);
		ck_assert_int_eq(wrk->hostarch, (2*i)%ARCH_HOST);
		ck_assert_int_eq(wrk->iscross, i%2);
		ck_assert_int_eq(wrk->cost, 2*i);
	}

	// End of real test.
	DESTROY_MY_SUBGROUP;
}
END_TEST

START_TEST(test_bworker_subgroup_remove)
{
	GIMME_A_SUBGROUP;
	int rc, lastrc = -1;


	// Now for the real test
	for (int i = 0; i < BWORKER_SUBGRP_SIZE; i++) {
		rc = bworker_subgroup_insert(subgroup, 0, i % ARCH_HOST,
				(2*i) % ARCH_HOST, i % 2, 2*i);
		ck_assert_int_gt(rc, lastrc);
		lastrc = rc;
	}
	rc = bworker_subgroup_remove(subgroup, 1);
	ck_assert_int_eq(rc, BWORKER_NUM_MAX);
	rc = bworker_subgroup_remove(subgroup, 0);
	ck_assert_int_eq(rc, BWORKER_NUM_MAX+1);

	bworker_subgroup_destroy(&subgroup);
	ck_assert_ptr_null(subgroup);
	subgroup = bworker_subgroup_init(group);
	ck_assert_ptr_nonnull(subgroup);
	lastrc = -1;
	for (int i = 0; i < BWORKER_SUBGRP_SIZE; i++) {
		rc = bworker_subgroup_insert(subgroup, i, i % ARCH_HOST,
				(2*i) % ARCH_HOST, i % 2, 2*i);
		ck_assert_int_gt(rc, lastrc);
		lastrc = rc;
	}
	for (int i = BWORKER_SUBGRP_SIZE-1; i >= 0; i--) {
		rc = bworker_subgroup_remove(subgroup, i);
		ck_assert_int_eq(rc, subgroup->min+i);
		ck_assert_int_eq(subgroup->num_workers, i);
	}
	ck_assert_int_eq(subgroup->num_workers, 0);

	// End of real test.
	DESTROY_MY_SUBGROUP;
}
END_TEST

START_TEST(test_bworker_job_assign)
{
	GIMME_A_GROUP;
	int rc;

	for (int i = 0; i < BWORKER_NUM_MAX; i++) {
		ck_assert_ptr_null(group->workers[i].job.name);
		ck_assert_ptr_null(group->workers[i].job.ver);
		ck_assert_int_eq(group->workers[i].job.arch, ARCH_NUM_MAX);
	}

	for (int i = 0; i < BWORKER_NUM_MAX; i++) {
		rc = bworker_job_assign(&(group->workers[i]), "Job A", "0.0.1", ARCH_X86_64_MUSL);
		ck_assert_int_eq(rc, 1);
		ck_assert_str_eq(group->workers[i].job.name, "Job A");
		ck_assert_str_eq(group->workers[i].job.ver, "0.0.1");
		ck_assert_int_eq(group->workers[i].job.arch, ARCH_X86_64_MUSL);

		rc = bworker_job_equal(&(group->workers[i]), "Job A", "0.0.1", ARCH_X86_64_MUSL);
		ck_assert_int_eq(rc, 1);
		rc = bworker_job_equal(&(group->workers[i]), "Job B", "0.0.2", ARCH_X86_64);
		ck_assert_int_eq(rc, 0);
		rc = bworker_job_equal(&(group->workers[i]), "Job B", "0.0.1", ARCH_X86_64_MUSL);
		ck_assert_int_eq(rc, 0);
		rc = bworker_job_equal(&(group->workers[i]), "Job AB", "0.0.1", ARCH_X86_64_MUSL);
		ck_assert_int_eq(rc, 0);
		rc = bworker_job_equal(&(group->workers[i]), "Job A", "0.0.11", ARCH_X86_64_MUSL);
		ck_assert_int_eq(rc, 0);
		rc = bworker_job_equal(&(group->workers[i]), "Job A", "0.0.1", ARCH_X86_64);
		ck_assert_int_eq(rc, 0);

		rc = bworker_job_assign(&(group->workers[i]), "Job B", "0.0.2", ARCH_X86_64);
		ck_assert_int_eq(rc, 1);
		ck_assert_str_eq(group->workers[i].job.name, "Job B");
		ck_assert_str_eq(group->workers[i].job.ver, "0.0.2");
		ck_assert_int_eq(group->workers[i].job.arch, ARCH_X86_64);

		rc = bworker_job_equal(&(group->workers[i]), "Job A", "0.0.1", ARCH_X86_64_MUSL);
		ck_assert_int_eq(rc, 0);
		rc = bworker_job_equal(&(group->workers[i]), "Job B", "0.0.2", ARCH_X86_64);
		ck_assert_int_eq(rc, 1);
		rc = bworker_job_equal(&(group->workers[i]), "Job B", "0.0.1", ARCH_X86_64_MUSL);
		ck_assert_int_eq(rc, 0);
		rc = bworker_job_equal(&(group->workers[i]), "Job AB", "0.0.1", ARCH_X86_64_MUSL);
		ck_assert_int_eq(rc, 0);
		rc = bworker_job_equal(&(group->workers[i]), "Job A", "0.0.11", ARCH_X86_64_MUSL);
		ck_assert_int_eq(rc, 0);
		rc = bworker_job_equal(&(group->workers[i]), "Job A", "0.0.1", ARCH_X86_64);
		ck_assert_int_eq(rc, 0);
	}
	for (int i = 0; i < BWORKER_NUM_MAX; i++) {
		bworker_job_remove(&(group->workers[i]));
		ck_assert_ptr_null(group->workers[i].job.name);
		ck_assert_ptr_null(group->workers[i].job.ver);
		ck_assert_int_eq(group->workers[i].job.arch, ARCH_NUM_MAX);

		rc = bworker_job_equal(&(group->workers[i]), "Job A", "0.0.1", ARCH_X86_64_MUSL);
		ck_assert_int_eq(rc, 0);
		rc = bworker_job_equal(&(group->workers[i]), "Job B", "0.0.2", ARCH_X86_64);
		ck_assert_int_eq(rc, 0);
		rc = bworker_job_equal(&(group->workers[i]), "Job B", "0.0.1", ARCH_X86_64_MUSL);
		ck_assert_int_eq(rc, 0);
		rc = bworker_job_equal(&(group->workers[i]), "Job AB", "0.0.1", ARCH_X86_64_MUSL);
		ck_assert_int_eq(rc, 0);
		rc = bworker_job_equal(&(group->workers[i]), "Job A", "0.0.11", ARCH_X86_64_MUSL);
		ck_assert_int_eq(rc, 0);
		rc = bworker_job_equal(&(group->workers[i]), "Job A", "0.0.1", ARCH_X86_64);
		ck_assert_int_eq(rc, 0);
		rc = bworker_job_equal(&(group->workers[i]), "Job A", "0.0.11", ARCH_NUM_MAX);
		ck_assert_int_eq(rc, 0);
	}

	DESTROY_MY_GROUP;
}
END_TEST

START_TEST(test_bworker_pkg_match)
{
	struct bworkgroup *group = NULL;
	group = bworker_group_new();
	ck_assert_ptr_nonnull(group);
	ck_assert_int_eq(group->num_workers, 0);
	ck_assert_int_eq(group->direct_use, 1);
	int rc = 0;

	struct pkg *mypkg = bpkg_init_oneoff("foo", "0.0.1", ARCH_X86_64_MUSL, 0, 0, 0);
	ck_assert_int_eq(mypkg->can_cross, 0);
	ck_assert_int_eq(mypkg->bootstrap, 0);
	ck_assert_int_eq(mypkg->restricted, 0);
	ck_assert_int_eq(mypkg->arch, ARCH_X86_64_MUSL);

	for (int i = 0; i < ARCH_HOST; i++) { // Addrs ARCH_HOST up
		rc = bworker_group_insert(group, ARCH_HOST+i, i, ARCH_X86_64, 0, 100);
		ck_assert_int_eq(rc, i);
	}
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs 0 -> ARCH_HOST
		rc = bworker_group_insert(group, i, i, ARCH_X86_64, 1, 100);
		ck_assert_int_eq(rc, ARCH_HOST+i);
	}
	for (int i = 0; i < group->num_workers; i++) {
		rc = bworker_pkg_match(&(group->workers[i]), mypkg);
		if (i == ARCH_X86_64_MUSL) // only one that should match
			ck_assert_int_eq(rc, 1);
		else
			ck_assert_int_eq(rc, 0);
	}
	bpkg_destroy(mypkg);
	bworker_group_destroy(&group);
	ck_assert_ptr_null(group);

	// Round two

	group = bworker_group_new();
	ck_assert_ptr_nonnull(group);
	ck_assert_int_eq(group->num_workers, 0);
	ck_assert_int_eq(group->direct_use, 1);

	mypkg = bpkg_init_oneoff("foo", "0.0.1", ARCH_X86_64, 1, 0, 0);
	ck_assert_int_eq(mypkg->can_cross, 1);
	ck_assert_int_eq(mypkg->bootstrap, 0);
	ck_assert_int_eq(mypkg->restricted, 0);
	ck_assert_int_eq(mypkg->arch, ARCH_X86_64);

	for (int i = 0; i < ARCH_HOST; i++) { // Addrs ARCH_HOST up
		rc = bworker_group_insert(group, ARCH_HOST+i, i, ARCH_X86_64, 0, 100);
		ck_assert_int_eq(rc, i);
	}
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs 0 -> ARCH_HOST
		rc = bworker_group_insert(group, i, i, ARCH_X86_64, 1, 100);
		ck_assert_int_eq(rc, ARCH_HOST+i);
	}
	for (int i = 0; i < group->num_workers; i++) {
		rc = bworker_pkg_match(&(group->workers[i]), mypkg);
		if (i == ARCH_X86_64 || i == ARCH_HOST + ARCH_X86_64) // only two that should match
			ck_assert_int_eq(rc, 1);
		else
			ck_assert_int_eq(rc, 0);
	}
	bpkg_destroy(mypkg);
	bworker_group_destroy(&group);

	// Round three

	group = bworker_group_new();
	ck_assert_ptr_nonnull(group);
	ck_assert_int_eq(group->num_workers, 0);
	ck_assert_int_eq(group->direct_use, 1);

	mypkg = bpkg_init_oneoff("foo", "0.0.1", ARCH_X86_64, 1, 0, 0);
	ck_assert_int_eq(mypkg->can_cross, 1);
	ck_assert_int_eq(mypkg->bootstrap, 0);
	ck_assert_int_eq(mypkg->restricted, 0);
	ck_assert_int_eq(mypkg->arch, ARCH_X86_64);

	for (int i = 0; i < ARCH_HOST; i++) { // Addrs ARCH_HOST up
		rc = bworker_group_insert(group, ARCH_HOST+i, i, ARCH_X86_64, 0, 100);
		ck_assert_int_eq(rc, i);
	}
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs 0 -> ARCH_HOST
		rc = bworker_group_insert(group, i, i, ARCH_X86_64, 1, 100);
		ck_assert_int_eq(rc, ARCH_HOST+i);
	}
	for (int i = 0; i < group->num_workers; i++) {
		rc = bworker_pkg_match(&(group->workers[i]), mypkg);
		if (i == ARCH_X86_64 || i == ARCH_HOST + ARCH_X86_64) // only two that should match
			ck_assert_int_eq(rc, 1);
		else
			ck_assert_int_eq(rc, 0);
	}
	rc = bworker_job_assign(&(group->workers[ARCH_X86_64]), "foo", "bar", ARCH_X86_64);
	ck_assert_int_eq(rc, 1);
	for (int i = 0; i < group->num_workers; i++) {
		rc = bworker_pkg_match(&(group->workers[i]), mypkg);
		if (i == ARCH_HOST + ARCH_X86_64)
			ck_assert_int_eq(rc, 1);
		else
			ck_assert_int_eq(rc, 0);
	}
	rc = bworker_job_assign(&(group->workers[ARCH_HOST + ARCH_X86_64]), "foo", "bar", ARCH_X86_64);
	ck_assert_int_eq(rc, 1);
	for (int i = 0; i < group->num_workers; i++) {
		rc = bworker_pkg_match(&(group->workers[i]), mypkg);
		ck_assert_int_eq(rc, 0);
	}
	bworker_job_remove(&(group->workers[ARCH_X86_64]));
	for (int i = 0; i < group->num_workers; i++) {
		rc = bworker_pkg_match(&(group->workers[i]), mypkg);
		if (i == ARCH_X86_64)
			ck_assert_int_eq(rc, 1);
		else
			ck_assert_int_eq(rc, 0);
	}
	bworker_job_remove(&(group->workers[ARCH_HOST + ARCH_X86_64]));
	for (int i = 0; i < group->num_workers; i++) {
		rc = bworker_pkg_match(&(group->workers[i]), mypkg);
		if (i == ARCH_X86_64 || i == ARCH_HOST + ARCH_X86_64 )
			ck_assert_int_eq(rc, 1);
		else
			ck_assert_int_eq(rc, 0);
	}
	bpkg_destroy(mypkg);
	bworker_group_destroy(&group);
	ck_assert_ptr_null(group);
}
END_TEST

START_TEST(test_bworker_group_pkg_matches)
{
	struct bworkgroup *group = NULL;
	group = bworker_group_new();
	ck_assert_ptr_nonnull(group);
	ck_assert_int_eq(group->num_workers, 0);
	ck_assert_int_eq(group->direct_use, 1);
	struct bworkermatch *matches = NULL;
	int rc = 0;

	struct pkg *mypkg = bpkg_init_oneoff("foo", "0.0.1", ARCH_X86_64_MUSL, 0, 0, 0);
	ck_assert_int_eq(mypkg->can_cross, 0);
	ck_assert_int_eq(mypkg->bootstrap, 0);
	ck_assert_int_eq(mypkg->restricted, 0);
	ck_assert_int_eq(mypkg->arch, ARCH_X86_64_MUSL);

	for (int i = 0; i < ARCH_HOST; i++) { // Addrs ARCH_HOST up
		rc = bworker_group_insert(group, ARCH_HOST+i, i, ARCH_X86_64, 0, 100);
		ck_assert_int_eq(rc, i);
	}
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs 0 -> ARCH_HOST
		rc = bworker_group_insert(group, i, i, ARCH_X86_64, 1, 100);
		ck_assert_int_eq(rc, ARCH_HOST+i);
	}
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs ARCH_HOST up
		rc = bworker_group_insert(group, (3*ARCH_HOST)+i, i, ARCH_X86_64, 0, 200);
		ck_assert_int_eq(rc, (2*ARCH_HOST)+i);
	}
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs 0 -> ARCH_HOST
		rc = bworker_group_insert(group, (2*ARCH_HOST)+i, i, ARCH_X86_64, 1, 200);
		ck_assert_int_eq(rc, (3*ARCH_HOST)+i);
	}
	matches = bworker_group_pkg_matches(group, mypkg);
	ck_assert_int_eq(matches->num, 2);
	ck_assert_ptr_eq(matches->workers[0], &(group->workers[ARCH_X86_64_MUSL]));
	ck_assert_ptr_eq(matches->workers[1], &(group->workers[(2*ARCH_HOST)+ARCH_X86_64_MUSL]));
	bworker_match_destroy(&matches);
	bpkg_destroy(mypkg);
	bworker_group_destroy(&group);
	ck_assert_ptr_null(group);

	// Round two

	group = bworker_group_new();
	ck_assert_ptr_nonnull(group);
	ck_assert_int_eq(group->num_workers, 0);
	ck_assert_int_eq(group->direct_use, 1);

	mypkg = bpkg_init_oneoff("foo", "0.0.1", ARCH_X86_64, 1, 0, 0);
	ck_assert_int_eq(mypkg->can_cross, 1);
	ck_assert_int_eq(mypkg->bootstrap, 0);
	ck_assert_int_eq(mypkg->restricted, 0);
	ck_assert_int_eq(mypkg->arch, ARCH_X86_64);

	for (int i = 0; i < ARCH_HOST; i++) { // Addrs ARCH_HOST up
		rc = bworker_group_insert(group, ARCH_HOST+i, i, ARCH_X86_64, 0, 100);
		ck_assert_int_eq(rc, i);
	}
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs 0 -> ARCH_HOST
		rc = bworker_group_insert(group, i, i, ARCH_X86_64, 1, 100);
		ck_assert_int_eq(rc, ARCH_HOST+i);
	}
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs ARCH_HOST up
		rc = bworker_group_insert(group, (3*ARCH_HOST)+i, i, ARCH_X86_64, 0, 200);
		ck_assert_int_eq(rc, (2*ARCH_HOST)+i);
	}
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs 0 -> ARCH_HOST
		rc = bworker_group_insert(group, (2*ARCH_HOST)+i, i, ARCH_X86_64, 1, 200);
		ck_assert_int_eq(rc, (3*ARCH_HOST)+i);
	}
	matches = bworker_group_pkg_matches(group, mypkg);
	ck_assert_int_eq(matches->num, 4);
	ck_assert_ptr_eq(matches->workers[0], &(group->workers[ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[1], &(group->workers[(1*ARCH_HOST)+ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[2], &(group->workers[(2*ARCH_HOST)+ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[3], &(group->workers[(3*ARCH_HOST)+ARCH_X86_64]));
	bworker_match_destroy(&matches);
	bpkg_destroy(mypkg);
	bworker_group_destroy(&group);

	// Round three

	group = bworker_group_new();
	ck_assert_ptr_nonnull(group);
	ck_assert_int_eq(group->num_workers, 0);
	ck_assert_int_eq(group->direct_use, 1);

	mypkg = bpkg_init_oneoff("foo", "0.0.1", ARCH_X86_64, 1, 0, 0);
	ck_assert_int_eq(mypkg->can_cross, 1);
	ck_assert_int_eq(mypkg->bootstrap, 0);
	ck_assert_int_eq(mypkg->restricted, 0);
	ck_assert_int_eq(mypkg->arch, ARCH_X86_64);

	for (int i = 0; i < ARCH_HOST; i++) { // Addrs ARCH_HOST up
		rc = bworker_group_insert(group, ARCH_HOST+i, i, ARCH_X86_64, 0, 100);
		ck_assert_int_eq(rc, i);
	}
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs 0 -> ARCH_HOST
		rc = bworker_group_insert(group, i, i, ARCH_X86_64, 1, 100);
		ck_assert_int_eq(rc, ARCH_HOST+i);
	}
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs ARCH_HOST up
		rc = bworker_group_insert(group, (3*ARCH_HOST)+i, i, ARCH_X86_64, 0, 200);
		ck_assert_int_eq(rc, (2*ARCH_HOST)+i);
	}
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs 0 -> ARCH_HOST
		rc = bworker_group_insert(group, (2*ARCH_HOST)+i, i, ARCH_X86_64, 1, 200);
		ck_assert_int_eq(rc, (3*ARCH_HOST)+i);
	}
	matches = bworker_group_pkg_matches(group, mypkg);
	ck_assert_int_eq(matches->num, 4);
	ck_assert_ptr_eq(matches->workers[0], &(group->workers[ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[1], &(group->workers[(1*ARCH_HOST)+ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[2], &(group->workers[(2*ARCH_HOST)+ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[3], &(group->workers[(3*ARCH_HOST)+ARCH_X86_64]));
	bworker_match_destroy(&matches);
	rc = bworker_job_assign(&(group->workers[ARCH_X86_64]), "foo", "bar", ARCH_X86_64);
	ck_assert_int_eq(rc, 1);
	matches = bworker_group_pkg_matches(group, mypkg);
	ck_assert_int_eq(matches->num, 3);
	ck_assert_ptr_eq(matches->workers[0], &(group->workers[(1*ARCH_HOST)+ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[1], &(group->workers[(2*ARCH_HOST)+ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[2], &(group->workers[(3*ARCH_HOST)+ARCH_X86_64]));
	bworker_match_destroy(&matches);
	rc = bworker_job_assign(&(group->workers[ARCH_HOST + ARCH_X86_64]), "foo", "bar", ARCH_X86_64);
	ck_assert_int_eq(rc, 1);
	matches = bworker_group_pkg_matches(group, mypkg);
	ck_assert_int_eq(matches->num, 2);
	ck_assert_ptr_eq(matches->workers[0], &(group->workers[(2*ARCH_HOST)+ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[1], &(group->workers[(3*ARCH_HOST)+ARCH_X86_64]));
	bworker_match_destroy(&matches);
	rc = bworker_job_assign(&(group->workers[(2*ARCH_HOST) + ARCH_X86_64]), "foo", "bar", ARCH_X86_64);
	ck_assert_int_eq(rc, 1);
	matches = bworker_group_pkg_matches(group, mypkg);
	ck_assert_int_eq(matches->num, 1);
	ck_assert_ptr_eq(matches->workers[0], &(group->workers[(3*ARCH_HOST)+ARCH_X86_64]));
	rc = bworker_job_assign(&(group->workers[(3*ARCH_HOST) + ARCH_X86_64]), "foo", "bar", ARCH_X86_64);
	ck_assert_int_eq(rc, 1);
	bworker_match_destroy(&matches);
	matches = bworker_group_pkg_matches(group, mypkg);
	ck_assert_int_eq(matches->num, 0);
	bworker_match_destroy(&matches);
	bworker_job_remove(&(group->workers[ARCH_X86_64]));
	matches = bworker_group_pkg_matches(group, mypkg);
	ck_assert_int_eq(matches->num, 1);
	ck_assert_ptr_eq(matches->workers[0], &(group->workers[ARCH_X86_64]));
	bworker_match_destroy(&matches);
	bworker_job_remove(&(group->workers[(1*ARCH_HOST) + ARCH_X86_64]));
	matches = bworker_group_pkg_matches(group, mypkg);
	ck_assert_int_eq(matches->num, 2);
	ck_assert_ptr_eq(matches->workers[0], &(group->workers[ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[1], &(group->workers[(1*ARCH_HOST)+ARCH_X86_64]));
	bworker_match_destroy(&matches);
	bworker_job_remove(&(group->workers[(2*ARCH_HOST) + ARCH_X86_64]));
	matches = bworker_group_pkg_matches(group, mypkg);
	ck_assert_int_eq(matches->num, 3);
	ck_assert_ptr_eq(matches->workers[0], &(group->workers[ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[1], &(group->workers[(1*ARCH_HOST)+ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[2], &(group->workers[(2*ARCH_HOST)+ARCH_X86_64]));
	bworker_match_destroy(&matches);
	bworker_job_remove(&(group->workers[(3*ARCH_HOST) + ARCH_X86_64]));
	matches = bworker_group_pkg_matches(group, mypkg);
	ck_assert_int_eq(matches->num, 4);
	ck_assert_ptr_eq(matches->workers[0], &(group->workers[ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[1], &(group->workers[(1*ARCH_HOST)+ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[2], &(group->workers[(2*ARCH_HOST)+ARCH_X86_64]));
	ck_assert_ptr_eq(matches->workers[3], &(group->workers[(3*ARCH_HOST)+ARCH_X86_64]));
	bworker_match_destroy(&matches);
	bpkg_destroy(mypkg);
	bworker_group_destroy(&group);
	ck_assert_ptr_null(group);
}
END_TEST

START_TEST(test_bworker_group_pkg_pick_match)
{
	struct bworkgroup *group = NULL;
	group = bworker_group_new();
	ck_assert_ptr_nonnull(group);
	ck_assert_int_eq(group->num_workers, 0);
	ck_assert_int_eq(group->direct_use, 1);
	struct bworker *match = NULL;
	int rc = 0;

	struct pkg *mypkg = bpkg_init_oneoff("foo", "0.0.1", ARCH_X86_64_MUSL, 0, 0, 0);
	ck_assert_int_eq(mypkg->can_cross, 0);
	ck_assert_int_eq(mypkg->bootstrap, 0);
	ck_assert_int_eq(mypkg->restricted, 0);
	ck_assert_int_eq(mypkg->arch, ARCH_X86_64_MUSL);
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs ARCH_HOST up
		rc = bworker_group_insert(group, ARCH_HOST+i, i, ARCH_X86_64, 0, 100);
		ck_assert_int_eq(rc, i);
	}
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs 0 -> ARCH_HOST
		rc = bworker_group_insert(group, i, i, ARCH_X86_64, 1, 100);
		ck_assert_int_eq(rc, ARCH_HOST+i);
	}
	for (int i = 0; i < ARCH_HOST; i++) {
		rc = bworker_group_insert(group, (3*ARCH_HOST)+i, i, ARCH_X86_64, 0, 200);
		ck_assert_int_eq(rc, (2*ARCH_HOST)+i);
	}
	for (int i = 0; i < ARCH_HOST; i++) {
		rc = bworker_group_insert(group, (2*ARCH_HOST)+i, i, ARCH_X86_64, 1, 200);
		ck_assert_int_eq(rc, (3*ARCH_HOST)+i);
	}

	match = bworker_group_pkg_pick_match(group, mypkg);
	ck_assert_ptr_eq(match, &(group->workers[ARCH_X86_64_MUSL]));
	rc = bworker_job_assign(match, "foo", "0.0.1", ARCH_X86_64_MUSL);
	ck_assert_int_eq(rc, 1);
	match = bworker_group_pkg_pick_match(group, mypkg);
	ck_assert_ptr_eq(match, &(group->workers[2*ARCH_HOST+ARCH_X86_64_MUSL]));
	rc = bworker_job_assign(match, "foo", "0.0.1", ARCH_X86_64_MUSL);
	ck_assert_int_eq(rc, 1);
	match = bworker_group_pkg_pick_match(group, mypkg);
	ck_assert_ptr_null(match);

	bworker_group_destroy(&group);
	ck_assert_ptr_null(group);
	group = bworker_group_new();
	ck_assert_ptr_nonnull(group);
	ck_assert_int_eq(group->num_workers, 0);
	ck_assert_int_eq(group->direct_use, 1);

	for (int i = 0; i < ARCH_HOST; i++) { // Addrs ARCH_HOST up
		rc = bworker_group_insert(group, ARCH_HOST+i, i, ARCH_X86_64, 0, 500);
		ck_assert_int_eq(rc, i);
	}
	for (int i = 0; i < ARCH_HOST; i++) { // Addrs 0 -> ARCH_HOST
		rc = bworker_group_insert(group, i, i, ARCH_X86_64, 1, 400);
		ck_assert_int_eq(rc, ARCH_HOST+i);
	}
	for (int i = 0; i < ARCH_HOST; i++) {
		rc = bworker_group_insert(group, (3*ARCH_HOST)+i, i, ARCH_X86_64, 0, 300);
		ck_assert_int_eq(rc, (2*ARCH_HOST)+i);
	}
	for (int i = 0; i < ARCH_HOST; i++) {
		rc = bworker_group_insert(group, (2*ARCH_HOST)+i, i, ARCH_X86_64, 1, 200);
		ck_assert_int_eq(rc, (3*ARCH_HOST)+i);
	}

	match = bworker_group_pkg_pick_match(group, mypkg);
	ck_assert_ptr_eq(match, &(group->workers[(2*ARCH_HOST) + ARCH_X86_64_MUSL]));
	rc = bworker_job_assign(match, "foo", "0.0.1", ARCH_X86_64_MUSL);
	ck_assert_int_eq(rc, 1);
	match = bworker_group_pkg_pick_match(group, mypkg);
	ck_assert_ptr_eq(match, &(group->workers[ARCH_X86_64_MUSL]));
	rc = bworker_job_assign(match, "foo", "0.0.1", ARCH_X86_64_MUSL);
	ck_assert_int_eq(rc, 1);
	match = bworker_group_pkg_pick_match(group, mypkg);
	ck_assert_ptr_null(match);

	bpkg_destroy(mypkg);
	bworker_group_destroy(&group);
	ck_assert_ptr_null(group);
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
	tcase_add_test(tc_core, test_bworker_subgroups);
	tcase_add_test(tc_core, test_bworker_subgroup_assign_slot);
	tcase_add_test(tc_core, test_bworker_subgroup_insert);
	tcase_add_test(tc_core, test_bworker_subgroup_remove);
	tcase_add_test(tc_core, test_bworker_pkg_match);
	tcase_add_test(tc_core, test_bworker_job_assign);
	tcase_add_test(tc_core, test_bworker_group_pkg_matches);
	tcase_add_test(tc_core, test_bworker_group_pkg_pick_match);
	suite_add_tcase(s, tc_core);

	return s;
}
