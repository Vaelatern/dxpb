/*
 * bdb.h
 *
 * A module to interact with a database. in persistent storage of package data.
 */

struct bdb_bound_params {
	sqlite3_stmt	*HASHROW;
	sqlite3_stmt	*HAD_ARCH;
	sqlite3_stmt	*ROW;
	sqlite3_stmt	*DELETE_SPECIFIC_ARCH;
	sqlite3_stmt	*DELETE_OTHER_ARCH;
	sqlite3_stmt	*LATEST_STATE;
	sqlite3		*DB;
	unsigned long	HashId;
	unsigned int	DO_NOT_USE_PARAMS : 1;
	int	del_spec_name;
	int	del_spec_arch;
	int	del_other_name;
	int	del_other_arch;
	int	find_arch;
	int	find_name;
	int	hash;
	int	hashid;
	int	name;
	int	version;
	int	arch;
	int	hostneeds;
	int	crosshostneeds;
	int	destneeds;
	int	crossdestneeds;
	int	cancross;
	int	broken;
	int	bootstrap;
	int	restricted;
};

int	bdb_write_hash_to_db(const char *, struct bdb_bound_params *);
int	bdb_write_tree(zhash_t *, struct bdb_bound_params *);
struct	bdb_bound_params *bdb_params_init();
struct	bdb_bound_params *bdb_params_init_ro();
void	bdb_params_destroy(struct bdb_bound_params **);
char 	*bdb_read_hash(struct bdb_bound_params *);
int	bdb_read_pkgs_to_graph(bgraph, struct bdb_bound_params *);
int	bdb_write_all(const char *, bgraph, const char *);
