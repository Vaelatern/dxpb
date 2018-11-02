/*
 * bdb.c
 *
 * A module to interact with a database. in persistent storage of package data.
 */

#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <czmq.h>
#include "pkgimport_msg.h"
#include "dxpb.h"
#include "bwords.h"
#include "bxpkg.h"
#include "bpkg.h"
#include "bgraph.h"
#include "bdb.h"

const char *BDB_TABLE_CREATE_STMT = "CREATE TABLE IF NOT EXISTS Packages \
				     ( \
				       name TEXT NOT NULL ON CONFLICT ROLLBACK, \
				       version TEXT NOT NULL ON CONFLICT ROLLBACK, \
				       arch TEXT NOT NULL ON CONFLICT ROLLBACK, \
				       hostneeds TEXT, \
				       crosshostneeds TEXT, \
				       destneeds TEXT, \
				       crossdestneeds TEXT, \
				       provides TEXT, \
				       cancross BOOLEAN NOT NULL ON CONFLICT ROLLBACK, \
				       broken BOOLEAN NOT NULL ON CONFLICT ROLLBACK, \
				       bootstrap BOOLEAN NOT NULL ON CONFLICT ROLLBACK, \
				       restricted BOOLEAN NOT NULL ON CONFLICT ROLLBACK, \
				       hashid INTEGER NOT NULL, \
				       PRIMARY KEY (name, arch) \
				     ) WITHOUT ROWID";

const char *BDB_METATABLE_CREATE_STMT = "CREATE TABLE IF NOT EXISTS State \
					 ( \
					   id INTEGER PRIMARY KEY AUTOINCREMENT, \
					   hash TEXT NOT NULL \
					 )";

const char *BDB_STATE_UPDATE_STMT = "INSERT INTO State \
				     (hash) VALUES ( $hash )";

const char *BDB_GET_LATEST_STATE = "SELECT MAX(id),hash FROM State";

const char *BDB_ROW_READ_ALL_STMT = "SELECT * FROM Packages";

const char *BDB_PKG_HAD_ARCH_STMT = "SELECT * FROM Packages WHERE \
				     (name = $name and arch = $arch)";

const char *BDB_PKG_DELARCH_STMT = "DELETE FROM Packages WHERE \
				     (name = $name and arch = $arch)";

const char *BDB_PKG_DELNOTARCH_STMT = "SELECT * FROM Packages WHERE \
				     (name = $name and arch != $arch)";

const char *BDB_ROW_CREATE_STMT = "INSERT OR REPLACE INTO Packages \
				   (name,version,arch,hostneeds,destneeds,\
				    crosshostneeds,crossdestneeds,provides, \
				    cancross,broken,bootstrap,restricted, \
				    hashid) \
				   VALUES ( \
						   $name, \
						   $version, \
						   $arch, \
						   $hostneeds, \
						   $destneeds, \
						   $crosshostneeds, \
						   $crossdestneeds, \
						   $provides, \
						   $cancross, \
						   $broken, \
						   $bootstrap, \
						   $restricted, \
						   $hashid \
					  )";

static void
bdb_del_stray_pkgs(struct pkg *curpkg,  struct bdb_bound_params *bound)
{
	/* The logic is simple. If a pkg is noarch, then it will have exactly
	 * one pkg entry, and that is of ARCH_NOARCH. If it is not noarch, it
	 * will have ARCH_TARGET for the noarch graph, and a pkg for every
	 * other arch. only_for_pkgs entries are simply marked broken, and also
	 * follow this rule. Thus:
	 *  - If a pkg is noarch, and it has ARCH_TARGET in the database, then
	 *    all entries of the pkg must be removed.
	 *  - If a pkg is !noarch, and it has ARCH_NOARCH, then that is the
	 *    only pkg entry in the database, and thus all entries of the pkg
	 *    must again be removed (only this time it's a shorter list).
	 * This is enforced in bgraph_insert_pkg() but must also be enforced
	 * anywhere that a copy of the package graph is placed, especially one
	 * where the copy does not get periodically destroyed and re-created.
	 * Vaelatern 2017-04-23
	 */
	int rc;
	sqlite3_stmt *deletion;
	sqlite3_stmt *find = bound->HAD_ARCH;
	assert(find);
	rc = sqlite3_bind_text(find, bound->find_name, curpkg->name, -1, SQLITE_STATIC);
	assert(rc == SQLITE_OK);
	if (curpkg->arch == ARCH_NOARCH) {
		rc = sqlite3_bind_text(find, bound->find_arch, pkg_archs_str[ARCH_TARGET], -1, SQLITE_STATIC);
		assert(rc == SQLITE_OK);
		deletion = bound->DELETE_OTHER_ARCH;
		assert(deletion);
		rc = sqlite3_bind_text(deletion, bound->del_other_name, curpkg->name, -1, SQLITE_STATIC);
		assert(rc == SQLITE_OK);
		rc = sqlite3_bind_text(deletion, bound->del_other_arch, pkg_archs_str[ARCH_NOARCH], -1, SQLITE_STATIC);
		assert(rc == SQLITE_OK);
	} else {
		rc = sqlite3_bind_text(find, bound->find_arch, pkg_archs_str[ARCH_NOARCH], -1, SQLITE_STATIC);
		assert(rc == SQLITE_OK);
		deletion = bound->DELETE_SPECIFIC_ARCH;
		assert(deletion);
		rc = sqlite3_bind_text(deletion, bound->del_spec_name, curpkg->name, -1, SQLITE_STATIC);
		assert(rc == SQLITE_OK);
		rc = sqlite3_bind_text(deletion, bound->del_spec_arch, pkg_archs_str[ARCH_NOARCH], -1, SQLITE_STATIC);
		assert(rc == SQLITE_OK);
	}
	rc = sqlite3_step(find);
	assert(rc != SQLITE_MISUSE);
	assert(rc != SQLITE_ERROR);
	if (rc == SQLITE_ROW) {
		rc = sqlite3_step(deletion);
		assert(rc == SQLITE_OK || rc == SQLITE_DONE);
	}
	rc = sqlite3_reset(find);
	assert(rc == SQLITE_OK);
	rc = sqlite3_reset(deletion);
	assert(rc == SQLITE_OK);
	return;
}

static int
bdb_write_pkg(struct pkg *curpkg, struct bdb_bound_params *bound)
{
	bdb_del_stray_pkgs(curpkg, bound);
	assert(curpkg);
	char *tmp;
	sqlite3_stmt *row = bound->ROW;
	int rc;

	rc = sqlite3_bind_text(row, bound->name, curpkg->name, -1, SQLITE_STATIC);
	assert(rc == SQLITE_OK);

	rc = sqlite3_bind_text(row, bound->version, curpkg->ver, -1, SQLITE_STATIC);
	assert(rc == SQLITE_OK);

	rc = sqlite3_bind_int(row, bound->cancross, curpkg->can_cross);
	assert(rc == SQLITE_OK);

	rc = sqlite3_bind_int(row, bound->broken, curpkg->broken);
	assert(rc == SQLITE_OK);

	rc = sqlite3_bind_int(row, bound->bootstrap, curpkg->bootstrap);
	assert(rc == SQLITE_OK);

	rc = sqlite3_bind_int(row, bound->restricted, curpkg->restricted);
	assert(rc == SQLITE_OK);

	rc = sqlite3_bind_int(row, bound->hashid, bound->HashId);
	assert(rc == SQLITE_OK);

	rc = sqlite3_bind_text(row, bound->arch, pkg_archs_str[curpkg->arch], -1, SQLITE_STATIC);
	assert(rc == SQLITE_OK);

	tmp = bwords_to_units(curpkg->wneeds_native_host);
	rc = sqlite3_bind_text(row, bound->hostneeds, tmp, -1, SQLITE_TRANSIENT);
	free(tmp);
	assert(rc == SQLITE_OK);

	tmp = bwords_to_units(curpkg->wneeds_cross_host);
	rc = sqlite3_bind_text(row, bound->crosshostneeds, tmp, -1, SQLITE_TRANSIENT);
	free(tmp);
	assert(rc == SQLITE_OK);

	tmp = bwords_to_units(curpkg->wneeds_native_target);
	rc = sqlite3_bind_text(row, bound->destneeds, tmp, -1, SQLITE_TRANSIENT);
	free(tmp);
	assert(rc == SQLITE_OK);

	tmp = bwords_to_units(curpkg->wneeds_cross_target);
	rc = sqlite3_bind_text(row, bound->crossdestneeds, tmp, -1, SQLITE_TRANSIENT);
	free(tmp);
	assert(rc == SQLITE_OK);

	tmp = bwords_to_units(curpkg->provides);
	rc = sqlite3_bind_text(row, bound->provides, tmp, -1, SQLITE_TRANSIENT);
	free(tmp);
	assert(rc == SQLITE_OK);

	rc = sqlite3_step(row);
	if (rc != SQLITE_OK && rc != SQLITE_DONE) {
		fprintf(stderr, "%d: %s\n", rc, sqlite3_errmsg(bound->DB));
		return ERR_CODE_BAD;
	}

	rc = sqlite3_reset(row);
	assert(rc == SQLITE_OK);
	return ERR_CODE_OK;
}

int
bdb_write_hash_to_db(const char *hash, struct bdb_bound_params *params)
{
	assert(hash[0] != '\0');
	assert(params->hash != 0);
	int rc;

	rc = sqlite3_bind_text(params->HASHROW, params->hash, hash, -1, SQLITE_TRANSIENT);
	assert(rc == SQLITE_OK);

	rc = sqlite3_step(params->HASHROW);
	if (rc != SQLITE_OK && rc != SQLITE_DONE) {
		fprintf(stderr, "%d: %s\n", rc, sqlite3_errmsg(params->DB));
		return ERR_CODE_BAD;
	}

	rc = sqlite3_reset(params->HASHROW);
	assert(rc == SQLITE_OK);

	rc = sqlite3_step(params->LATEST_STATE);
	assert(rc != SQLITE_MISUSE);
	assert(rc != SQLITE_ERROR);
	if (rc == SQLITE_ROW) {
		params->HashId = sqlite3_column_int(params->LATEST_STATE, 0);
	} else if (rc != SQLITE_DONE) {
		perror("Whups, database failed");
		exit(ERR_CODE_BADDB);
	}

	rc = sqlite3_reset(params->LATEST_STATE);
	assert(rc == SQLITE_OK);

	return ERR_CODE_OK;
}

static int
write_pkgs_to_db(zhash_t *pkgs, struct bdb_bound_params *params)
{
	struct pkg *curpkg;
	curpkg = NULL;
	int rc;

	assert(params->DB != NULL);
	assert(params->ROW != NULL);
	assert(params->HAD_ARCH != NULL);
	assert(params->DELETE_SPECIFIC_ARCH != NULL);
	assert(params->DELETE_OTHER_ARCH != NULL);
	assert(params->name != 0);
	assert(params->version != 0);
	assert(params->cancross != 0);
	assert(params->broken != 0);
	assert(params->hashid != 0);
	assert(params->restricted != 0);
	assert(params->bootstrap != 0);
	assert(params->arch != 0);
	assert(params->hostneeds != 0);
	assert(params->crosshostneeds != 0);
	assert(params->destneeds != 0);
	assert(params->crossdestneeds != 0);
	assert(params->provides != 0);

	for (curpkg = zhash_first(pkgs); curpkg != NULL;
			curpkg = zhash_next(pkgs)) {
		rc = bdb_write_pkg(curpkg, params);
		if (rc == ERR_CODE_BAD)
			return rc;
	}
	return ERR_CODE_OK;
}

int
bdb_write_tree(zhash_t *pkgs, struct bdb_bound_params *params)
{
	assert(pkgs);
	assert(params);
	assert(params->DB);
	assert(params->ROW);
	zhash_t *curpkgs;
	int rc,
	    retVal = ERR_CODE_OK;

	for (curpkgs = zhash_first(pkgs); curpkgs != NULL; curpkgs = zhash_next(pkgs)) {
		if (bpkg_enum_lookup(zhash_cursor(pkgs)) == ARCH_NUM_MAX)
			continue;
		rc = write_pkgs_to_db(curpkgs, params);
		if (rc != ERR_CODE_OK)
			retVal = ERR_CODE_BAD;
	}

	return retVal;
}

static struct bdb_bound_params *
bdb_params_get_new()
{
	struct bdb_bound_params *retVal;
	retVal = calloc(1, sizeof(struct bdb_bound_params));
	if (retVal == NULL) {
		perror("Could not allocate new params");
		exit(ERR_CODE_NOMEM);
	}
	return retVal;
}

static void
bdb_params_init_rows(struct bdb_bound_params *params)
{
	/* Set indexes for DELETE_OTHER_ARCH */
	params->del_other_name = sqlite3_bind_parameter_index(params->DELETE_OTHER_ARCH, "$name");
	params->del_other_arch = sqlite3_bind_parameter_index(params->DELETE_OTHER_ARCH, "$arch");

	/* Set indexes for DELETE_SPECIFIC_ARCH */
	params->del_spec_name = sqlite3_bind_parameter_index(params->DELETE_SPECIFIC_ARCH, "$name");
	params->del_spec_arch = sqlite3_bind_parameter_index(params->DELETE_SPECIFIC_ARCH, "$arch");

	/* Set indexes for HAD_ARCH */
	params->find_name = sqlite3_bind_parameter_index(params->HAD_ARCH, "$name");
	params->find_arch = sqlite3_bind_parameter_index(params->HAD_ARCH, "$arch");

	/* Set indexes for HASHROW*/
	params->hash = sqlite3_bind_parameter_index(params->HASHROW, "$hash");

	/* Set indexes for ROW*/
	params->name = sqlite3_bind_parameter_index(params->ROW, "$name");
	params->version = sqlite3_bind_parameter_index(params->ROW, "$version");
	params->cancross = sqlite3_bind_parameter_index(params->ROW, "$cancross");
	params->broken = sqlite3_bind_parameter_index(params->ROW, "$broken");
	params->hashid = sqlite3_bind_parameter_index(params->ROW, "$hashid");
	params->restricted = sqlite3_bind_parameter_index(params->ROW, "$restricted");
	params->bootstrap = sqlite3_bind_parameter_index(params->ROW, "$bootstrap");
	params->arch = sqlite3_bind_parameter_index(params->ROW, "$arch");
	params->hostneeds = sqlite3_bind_parameter_index(params->ROW, "$hostneeds");
	params->crosshostneeds = sqlite3_bind_parameter_index(params->ROW, "$crosshostneeds");
	params->destneeds = sqlite3_bind_parameter_index(params->ROW, "$destneeds");
	params->crossdestneeds = sqlite3_bind_parameter_index(params->ROW, "$crossdestneeds");
	params->provides = sqlite3_bind_parameter_index(params->ROW, "$provides");
}

static void
bdb_params_init_db(const char *db_path, struct bdb_bound_params *params)
{
	int rc;
	sqlite3_stmt *table_stmt = NULL,
		     *hash_table_stmt = NULL;

	rc = sqlite3_open_v2(db_path, &(params->DB), SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);
	assert(params->DB);
	if (params->DB == NULL || rc != SQLITE_OK)
		return;

	rc = sqlite3_prepare_v2(params->DB, BDB_TABLE_CREATE_STMT, strlen(BDB_TABLE_CREATE_STMT)+1, &table_stmt, NULL);
	assert(rc == SQLITE_OK);
	rc = sqlite3_step(table_stmt);
	assert(rc == SQLITE_DONE);
	rc = sqlite3_reset(table_stmt);
	assert(rc == SQLITE_OK);
	rc =sqlite3_finalize(table_stmt);
	assert(rc == SQLITE_OK);

	rc = sqlite3_prepare_v2(params->DB, BDB_METATABLE_CREATE_STMT, strlen(BDB_METATABLE_CREATE_STMT)+1, &hash_table_stmt, NULL);
	assert(rc == SQLITE_OK);
	rc = sqlite3_step(hash_table_stmt);
	assert(rc == SQLITE_DONE);
	rc = sqlite3_reset(hash_table_stmt);
	assert(rc == SQLITE_OK);
	rc =sqlite3_finalize(hash_table_stmt);
	assert(rc == SQLITE_OK);

	rc = sqlite3_prepare_v2(params->DB, BDB_ROW_CREATE_STMT, strlen(BDB_ROW_CREATE_STMT)+1, &(params->ROW), NULL);
	assert(rc == SQLITE_OK);

	rc = sqlite3_prepare_v2(params->DB, BDB_PKG_HAD_ARCH_STMT, strlen(BDB_PKG_HAD_ARCH_STMT)+1, &(params->HAD_ARCH), NULL);
	assert(rc == SQLITE_OK);

	rc = sqlite3_prepare_v2(params->DB, BDB_STATE_UPDATE_STMT, strlen(BDB_STATE_UPDATE_STMT)+1, &(params->HASHROW), NULL);
	assert(rc == SQLITE_OK);

	rc = sqlite3_prepare_v2(params->DB, BDB_GET_LATEST_STATE, strlen(BDB_GET_LATEST_STATE)+1, &(params->LATEST_STATE), NULL);
	assert(rc == SQLITE_OK);

	rc = sqlite3_prepare_v2(params->DB, BDB_PKG_DELARCH_STMT, strlen(BDB_PKG_DELARCH_STMT)+1, &(params->DELETE_SPECIFIC_ARCH), NULL);
	assert(rc == SQLITE_OK);

	rc = sqlite3_prepare_v2(params->DB, BDB_PKG_DELNOTARCH_STMT, strlen(BDB_PKG_DELNOTARCH_STMT)+1, &(params->DELETE_OTHER_ARCH), NULL);
	assert(rc == SQLITE_OK);

}

static int
bdb_params_init_db_ro(const char *db_path, struct bdb_bound_params *params)
{
	int rc;

	rc = sqlite3_open_v2(db_path, &(params->DB), SQLITE_OPEN_READONLY, NULL);

	/* Please make this more judgemental over time. Maybe also log the
	error or some such. If the file can not be read, that's a different
	sort of problem. SADDB would be a good return code for that.
	2017-04-23 */
	if (rc != SQLITE_OK)
		return ERR_CODE_NODB;
	assert(params->DB);
	assert(rc == SQLITE_OK);

	rc = sqlite3_prepare_v2(params->DB, BDB_ROW_READ_ALL_STMT, strlen(BDB_ROW_READ_ALL_STMT)+1, &(params->ROW), NULL);
	assert(rc == SQLITE_OK);

	rc = sqlite3_prepare_v2(params->DB, BDB_GET_LATEST_STATE, strlen(BDB_GET_LATEST_STATE)+1, &(params->LATEST_STATE), NULL);
	assert(rc == SQLITE_OK);

	return ERR_CODE_OK;
}

struct bdb_bound_params *
bdb_params_init(const char *db_path)
{
	struct bdb_bound_params *params = bdb_params_get_new();
	bdb_params_init_db(db_path, params);
	if (params->DB == NULL)
		params->DO_NOT_USE_PARAMS = 1;
	else
		bdb_params_init_rows(params);
	return params;
}

struct bdb_bound_params *
bdb_params_init_ro(const char *db_path)
{
	struct bdb_bound_params *params = bdb_params_get_new();
	int rc = bdb_params_init_db_ro(db_path, params);
	if (rc == ERR_CODE_OK)
		bdb_params_init_rows(params);
	else
		params->DO_NOT_USE_PARAMS = 1;
	return params;
}

void
bdb_params_destroy(struct bdb_bound_params **params)
{
	int rc;
	if ((*params)->HASHROW) {
		rc = sqlite3_finalize((*params)->HASHROW);
		assert(rc == SQLITE_OK);
	}
	if ((*params)->ROW) {
		rc = sqlite3_finalize((*params)->ROW);
		assert(rc == SQLITE_OK);
	}
	if ((*params)->HAD_ARCH) {
		rc = sqlite3_finalize((*params)->HAD_ARCH);
		assert(rc == SQLITE_OK);
	}
	if ((*params)->DELETE_SPECIFIC_ARCH) {
		rc = sqlite3_finalize((*params)->DELETE_SPECIFIC_ARCH);
		assert(rc == SQLITE_OK);
	}
	if ((*params)->DELETE_OTHER_ARCH) {
		rc = sqlite3_finalize((*params)->DELETE_OTHER_ARCH);
		assert(rc == SQLITE_OK);
	}
	if ((*params)->LATEST_STATE) {
		rc = sqlite3_finalize((*params)->LATEST_STATE);
		assert(rc == SQLITE_OK);
	}
	rc = sqlite3_close_v2((*params)->DB);
	assert(rc == SQLITE_OK);
	*params = NULL;
}

// Quite optimized here. But it's good.
static void
bdb_fill_from_column(struct bdb_bound_params *params, struct pkg *pkg, int col)
{
	const char *colname = sqlite3_column_name(params->ROW, col);
	unsigned const char *tmp = NULL; // UTF-8 breakage here. We don't support it.
	switch(colname[0]) {
	case 'a': // arch
		tmp = sqlite3_column_text(params->ROW, col);
		pkg->arch = bpkg_enum_lookup((const char *)tmp);
		break;
	case 'b': // broken or bootstrap
		switch(colname[1]) {
		case 'o': // bootstrap
			pkg->bootstrap = sqlite3_column_int(params->ROW, col);
			break;
		case 'r': // broken
			pkg->broken = sqlite3_column_int(params->ROW, col);
			break;
		default:
			exit(ERR_CODE_BADDB);
		}
		break;
	case 'c': // cancross, crossdestneeds, crosshostneeds
		switch(colname[5]) {
		case 'o': // cancross
			pkg->can_cross = sqlite3_column_int(params->ROW, col);
			break;
		case 'd': // crossdestneeds
			tmp = sqlite3_column_text(params->ROW, col);
			pkg->wneeds_cross_target = bwords_from_units(
					(const char *)tmp);
			break;
		case 'h': // crosshostneeds
			tmp = sqlite3_column_text(params->ROW, col);
			pkg->wneeds_cross_host = bwords_from_units(
					(const char *)tmp);
			break;
		default:
			exit(ERR_CODE_BADDB);
		}
		break;
	case 'd': //destneeds
		tmp = sqlite3_column_text(params->ROW, col);
		pkg->wneeds_native_target =
					bwords_from_units((const char *)tmp);
		break;
	case 'h': // hashid hostneeds
		switch(colname[1]) {
		case 'a': // hashid
			break;
		case 'o': // hostneeds
			tmp = sqlite3_column_text(params->ROW, col);
			pkg->wneeds_native_host =
				bwords_from_units((const char *)tmp);
			break;
		default:
			exit(ERR_CODE_BADDB);
		}
		break;
	case 'n': // name
		tmp = sqlite3_column_text(params->ROW, col);
		pkg->name = strdup((const char *)tmp);
		break;
	case 'p': // provides
		tmp = sqlite3_column_text(params->ROW, col);
		pkg->provides = bwords_from_units((const char *)tmp);
		break;
	case 'r': // restricted
		pkg->restricted = sqlite3_column_int(params->ROW, col);
		break;
	case 'v': // version
		tmp = sqlite3_column_text(params->ROW, col);
		pkg->ver = strdup((const char *)tmp);
		break;
	default:
		exit(ERR_CODE_BADDB);
	}
}

static struct pkg *
bdb_pkg_from_step(struct bdb_bound_params *params)
{
	assert(params);
	struct pkg *retVal = NULL;
	const int numCols = sqlite3_column_count(params->ROW);
	int rc = sqlite3_step(params->ROW);
	assert(rc != SQLITE_MISUSE);
	assert(rc != SQLITE_ERROR);
	if (rc == SQLITE_ROW) {
		retVal = bpkg_init_basic();
		for (int col = 0; col < numCols; col++)
			bdb_fill_from_column(params, retVal, col);
	} // else return NULL:
	if (sqlite3_errcode(params->DB) == SQLITE_NOMEM) {
		perror("Sqlite encounted memory problems");
		exit(ERR_CODE_NOMEM);
	}
	return retVal;
}

// TODO: Fix the references to the 1st and 0th row to be better.
// Maybe some form of select statement that allows parameter indexes to be 
// found rather than lots of strcmps
// Vaelatern 2017-05-01
char *
bdb_read_hash(struct bdb_bound_params *params)
{
	if (params->DO_NOT_USE_PARAMS != 0)
		return NULL;
	char *hash = NULL;
	const unsigned char *tmp; // UTF-8 breakage here. We don't support it.
	int rc;

	rc = sqlite3_step(params->LATEST_STATE);
	assert(rc != SQLITE_MISUSE);
	assert(rc != SQLITE_ERROR);
	if (rc == SQLITE_ROW) {
		tmp = sqlite3_column_text(params->LATEST_STATE, 1); // the 1st row
		if (tmp != NULL)
			hash = strdup((const char *)tmp);
		if (sqlite3_errcode(params->DB) == SQLITE_NOMEM) {
			perror("Sqlite encounted memory problems");
			exit(ERR_CODE_NOMEM);
		}
		if (hash == NULL && tmp != NULL) {
			perror("We encounted memory problems duplicating strings from sqlite");
			exit(ERR_CODE_NOMEM);
		}
		params->HashId = sqlite3_column_int(params->LATEST_STATE, 0); // the 0th row
	} // Else hash is null and we return it;

	rc = sqlite3_reset(params->LATEST_STATE);
	assert(rc == SQLITE_OK);
	return hash;
}

int
bdb_read_pkgs_to_graph(bgraph *grph, struct bdb_bound_params *params)
{
	if (params->DO_NOT_USE_PARAMS)
		return ERR_CODE_BAD;
	assert(params);
	assert(params->ROW);
	assert(!*grph);
	*grph = bgraph_new();
	struct pkg *tmp;

	while ((tmp = bdb_pkg_from_step(params)) != NULL)
		bgraph_insert_pkg(*grph, tmp);

	return ERR_CODE_OK;
}

int
bdb_write_all(const char *db_path, bgraph grph, const char *hash)
{
	assert(db_path);
	assert(grph);
	assert(hash);
	struct bdb_bound_params *params = bdb_params_init(db_path);
	assert(params->DB);
	sqlite3_exec(params->DB, "BEGIN TRANSACTION", NULL, NULL, NULL);
	int rc = bdb_write_hash_to_db(hash, params);
	if (rc != ERR_CODE_OK) {
		perror("rc != ERR_CODE_OK for writing hash");
		/* TODO: XXX: MUST HAVE a provision to work around this!
		 * 2017-03-14
		 * 2017-05-02
		 * 2018-09-30 */
	} else {
		rc = bdb_write_tree(grph, params);
		if (rc != ERR_CODE_OK) {
			; /* TODO: XXX: MUST HAVE a provision to work around this!
			     Likely a rollback of the database and not continuing with
			     this function call except to destroy params.
			     2017-03-14
			     2018-09-30 */
		}
		sqlite3_exec(params->DB, "COMMIT TRANSACTION", NULL, NULL, NULL);
	}
	bdb_params_destroy(&params);
	return ERR_CODE_OK;
}
