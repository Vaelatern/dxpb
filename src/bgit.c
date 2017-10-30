/*
 * bgit.c
 *
 * Module for working with a git repo.
 */
#define _POSIX_C_SOURCE 200809L

#include <string.h>
#include <assert.h>
#include <stdio.h>
#include <git2.h>
#include <czmq.h>
#include "dxpb.h"
#include "bwords.h"
#include "bgit.h"
#include "bxpkg.h"
#include "bxsrc.h"

/* Local only */
static	int bgit_cb_extract_pkgnames(const git_diff_delta *, float, void *);
static	void bgit_ff(git_repository *);
static	bwords bgit_ff_get_changed_pkgs(const char *);

// TODO: This should be changed to _FORCE after the xbps-src patch
// is merged. Until then, this is necessary.
#define BGIT_CHECKOUT_STRATEGY GIT_CHECKOUT_SAFE

static int
bgit_cb_extract_pkgnames(const git_diff_delta *delta, float progress, void *payload)
{
	assert(progress >= 0 && progress <= 1);
	assert(payload);
	assert(delta);
	bwords *actual_load = payload;
	const char *oldname = (delta->old_file).path;
	const char *newname = (delta->new_file).path;
	char *slash;
	slash = strchr(oldname, '/');
	if (slash != NULL)
		*slash = '\0';
	slash = strchr(newname, '/');
	if (slash != NULL)
		*slash = '\0';
	*actual_load = bwords_append_word(*actual_load, oldname, 0);
	if (strcmp(oldname, newname) != 0)
		*actual_load = bwords_append_word(*actual_load, newname, 0);
	return 0;
}

static void
bgit_ff(git_repository *repo)
{
	assert(repo);
	int rc;
	git_libgit2_init();
	git_remote *remote = NULL;
	git_strarray refspecs[1];
	git_reference *newref = NULL, *lastref = NULL;
	git_tree *newtree = NULL;
	git_commit *commit = NULL;
	git_oid oid;
	git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
	opts.checkout_strategy = BGIT_CHECKOUT_STRATEGY;

	/* Fetch the origin at dxpb-remote, updating refs */
	rc = git_remote_lookup(&remote, repo, "dxpb-remote");
	assert(rc == 0);
	rc = git_remote_get_fetch_refspecs(refspecs, remote);
	assert(rc == 0); // According to reading the code, not according to docs!
	rc = git_remote_fetch(remote, refspecs, NULL, NULL);
	git_strarray_free(refspecs);
	assert(rc == 0);

	/* Figure out what the new tree is */
	rc = git_reference_name_to_id(&oid, repo, "refs/remotes/dxpb-remote/master");
	assert(rc == 0);
	rc = git_commit_lookup(&commit, repo, &oid);
	assert(rc == 0);
	rc = git_commit_tree(&newtree, commit);
	assert(rc == 0);

	/* Checkout the new tree */
	rc = git_checkout_tree(repo, (git_object *)newtree, &opts);
	assert(rc == 0);

	/* Move the references to make master mirror dxpb-remote/master */
	rc = git_repository_head(&newref, repo);
	assert(rc == 0);
	rc = git_reference_set_target(&lastref, newref, &oid, "fast-forwarding");
	assert(rc == 0);

	/* Cleanup */
	rc = git_repository_state_cleanup(repo);
	assert(rc == 0);

	git_commit_free(commit);
	git_reference_free(lastref);
	git_reference_free(newref);
	git_tree_free(newtree);
	git_remote_free(remote);
	git_libgit2_shutdown();
}

void
bgit_just_ff(const char *repopath)
{
	int rc;
	git_repository *repo = NULL;

	git_libgit2_init();
	rc = git_repository_open(&repo, repopath);
	assert(rc == 0);

	/* Fast forward the repo */
	bgit_ff(repo);

	git_repository_free(repo);
	git_libgit2_shutdown();
}

static bwords
bgit_ff_get_changed_pkgs(const char *repopath)
{
	int rc;
	bwords ptr_to_words = bwords_new();
	git_tree *old_root, *new_root, *old_cmp_root, *new_cmp_root;
	git_tree_entry *tmp;
	git_diff *diff = NULL;
	git_repository *repo = NULL;
	git_oid oid;
	const git_oid *subtree_oid;
	git_commit *commit = NULL;
	old_root = new_root = old_cmp_root = new_cmp_root = NULL;
	tmp = NULL;

	git_libgit2_init();
	rc = git_repository_open(&repo, repopath);
	assert(rc == 0);

	/* Get tree of current repo */
	rc = git_reference_name_to_id(&oid, repo, "HEAD");
	assert(rc == 0);
	rc = git_commit_lookup(&commit, repo, &oid);
	assert(rc == 0);
	rc = git_commit_tree(&old_root, commit);
	assert(rc == 0);
	git_commit_free(commit);
	/* Narrow focus to just srcpkgs/ */
	rc = git_tree_entry_bypath(&tmp, old_root, "srcpkgs");
	assert(rc == 0);
	subtree_oid = git_tree_entry_id(tmp);
	rc = git_tree_lookup(&old_cmp_root, repo, subtree_oid);
	assert(rc == 0);
	git_tree_entry_free(tmp);

	/* Fast forward the repo */
	bgit_ff(repo);

	/* Get tree of the now-current repo */
	rc = git_reference_name_to_id(&oid, repo, "HEAD");
	assert(rc == 0);
	rc = git_commit_lookup(&commit, repo, &oid);
	assert(rc == 0);
	rc = git_commit_tree(&new_root, commit);
	assert(rc == 0);
	git_commit_free(commit);
	/* Narrow focus to just srcpkgs/ */
	rc = git_tree_entry_bypath(&tmp, new_root, "srcpkgs");
	assert(rc == 0);
	subtree_oid = git_tree_entry_id(tmp);
	rc = git_tree_lookup(&new_cmp_root, repo, subtree_oid);
	assert(rc == 0);
	git_tree_entry_free(tmp);

	/* And for every difference, take actions */
	rc = git_diff_tree_to_tree(&diff, repo, old_cmp_root, new_cmp_root, NULL);
	assert(rc == 0); // According to reading the code, not according to docs!
	rc = git_diff_foreach(diff, bgit_cb_extract_pkgnames, NULL, NULL, NULL, &ptr_to_words);
	assert(rc == 0);

	git_diff_free(diff);
	git_tree_free(old_cmp_root);
	git_tree_free(old_root);
	git_tree_free(new_cmp_root);
	git_tree_free(new_root);
	git_repository_free(repo);
	git_libgit2_shutdown();
	return ptr_to_words;
}

void
bgit_proc_changed_pkgs(const char *repopath, const char *xbps_src, int (*cb)(void*, char*), void *topass)
{
	bwords words, pkgs, final;
	words = pkgs = final = NULL;
	size_t i, j;
	char *tmp;
	int rc;
	words = bgit_ff_get_changed_pkgs(repopath);
	if (words == NULL)
		return;
	words = bwords_deduplicate(words);
	for (i = 0; i < words->num_words; i++) {
		pkgs = bxsrc_query_pkgnames(xbps_src, words->words[i]);
		for (j = 0; j < pkgs->num_words; j++) {
			final = bwords_append_word(final, pkgs->words[j], 1);
		}
		bwords_destroy(&pkgs, 0);
	}
	bwords_destroy(&words, 1);
	final = bwords_deduplicate(final);
	for (i = 0; i < final->num_words; i++) {
		tmp = strdup(final->words[i]);
		if (tmp == NULL) {
			perror("Failed to copy a word to be called back");
			exit(ERR_CODE_NOMEM);
		}

		rc = (*cb)(topass, tmp);
		if (rc != 0) {
			exit(ERR_CODE_BAD);
		}
	}
	bwords_destroy(&final, 1); // we strduped, so it's ok.
}

char *
bgit_get_head_hash(const char *repopath)
{
	assert(GIT_OID_HEXSZ > 0);
	char *retVal;
	git_oid oid;
	git_repository *repo = NULL;

	if ((retVal = malloc((GIT_OID_HEXSZ+1)*sizeof(char))) == NULL) {
		perror("Could not allocate memory for a shasum");
		exit(ERR_CODE_NOMEM);
	}

	git_libgit2_init();
	int rc = git_repository_open(&repo, repopath);
	assert(rc == 0);
	rc = git_reference_name_to_id(&oid, repo, "HEAD");
	assert(rc == 0);
	git_oid_tostr(retVal, GIT_OID_HEXSZ+1, &oid);
	assert(retVal[0] != '\0');
	assert(retVal[GIT_OID_HEXSZ] == '\0');
	git_repository_free(repo);
	git_libgit2_shutdown();

	return retVal;
}

int
bgit_checkout_hash(const char *repopath, const char *hash)
{
	int retVal = 0;
	int rc = 0;
	git_oid hashoid, headoid, mergeoid;
	git_repository *repo = NULL;
	git_tree *newtree = NULL;
	git_commit *commit = NULL;
	git_reference *newref = NULL, *lastref = NULL;
	git_checkout_options opts = GIT_CHECKOUT_OPTIONS_INIT;
	opts.checkout_strategy = BGIT_CHECKOUT_STRATEGY;

	git_libgit2_init();
	rc = git_repository_open(&repo, repopath);
	assert(rc == 0);
	rc = git_reference_name_to_id(&headoid, repo, "HEAD");
	assert(rc == 0);
	rc = git_oid_fromstr(&hashoid, hash);
	if (rc != 0) {
		perror("Couldn't get an oid from hash");
		retVal = -2;
		goto cleanup;
	}
	if (git_oid_equal(&hashoid, &headoid) == 0) {
		retVal = 0; // Success. We are at that hash too.
		goto cleanup;
	} else if ((rc = git_merge_base(&mergeoid, repo, &headoid, &hashoid)) != 0) {
		/* Unsuccessful */
		if (rc == GIT_ENOTFOUND) {
			perror("Asked to check out a hash not in our history");
			retVal = -2;
			goto cleanup;
		} else {
			perror("Some error occured looking up hashes");
			retVal = -2;
			goto cleanup;
		}
	} else {
		/* Since we want to check out the commit under mergeoid */
		rc = git_commit_lookup(&commit, repo, &mergeoid);
		assert(rc == 0);
		rc = git_commit_tree(&newtree, commit);
		assert(rc == 0);

		/* Checkout the new tree */
		rc = git_checkout_tree(repo, (git_object *)newtree, &opts);
		assert(rc == 0);

		/* Move the master branch to the hashoid */
		rc = git_repository_head(&newref, repo);
		assert(rc == 0);
		rc = git_reference_set_target(&lastref, newref, &mergeoid,
				"revert to expected hash");
		assert(rc == 0);

		/* Cleanup */
		rc = git_repository_state_cleanup(repo);
		assert(rc == 0);
		git_commit_free(commit);
		git_tree_free(newtree);
		git_reference_free(lastref);
		git_reference_free(newref);
	}
cleanup:
	git_repository_free(repo);
	git_libgit2_shutdown();
	return retVal;
}
