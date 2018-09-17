#ifndef CAPN_9DABF9CB287E69A
#define CAPN_9DABF9CB287E69A
/* AUTO GENERATED - DO NOT EDIT */
#include <capnp_c.h>

#if CAPN_VERSION != 1
#error "version mismatch between capnp_c.h and generated code"
#endif

#ifndef capnp_nowarn
# ifdef __GNUC__
#  define capnp_nowarn __extension__
# else
#  define capnp_nowarn
# endif
#endif

#include "../capnproto/compiler/c.capnp.h"

#ifdef __cplusplus
extern "C" {
#endif

struct LogEntry;
struct PkgSpec;
struct Worker;

typedef struct {capn_ptr p;} LogEntry_ptr;
typedef struct {capn_ptr p;} PkgSpec_ptr;
typedef struct {capn_ptr p;} Worker_ptr;

typedef struct {capn_ptr p;} LogEntry_list;
typedef struct {capn_ptr p;} PkgSpec_list;
typedef struct {capn_ptr p;} Worker_list;

enum Arch {
	Arch_noarch = 0,
	Arch_aarch64 = 1,
	Arch_aarch64Musl = 2,
	Arch_armv6l = 3,
	Arch_armv6lMusl = 4,
	Arch_armv7l = 5,
	Arch_armv7lMusl = 6,
	Arch_i686 = 7,
	Arch_i686Musl = 8,
	Arch_x8664 = 9,
	Arch_x8664Musl = 10,
	Arch_armv5Tel = 11,
	Arch_armv5TelMusl = 12,
	Arch_mipselMusl = 13,
	Arch_virtual = 14
};
enum LogEntry_l_which {
	LogEntry_l_pkgImported = 0,
	LogEntry_l_pkgAddedToGraph = 1,
	LogEntry_l_graphSaved = 2,
	LogEntry_l_pkgImportedForDeletion = 3,
	LogEntry_l_graphRead = 4,
	LogEntry_l_queueSelected = 5,
	LogEntry_l_workerAddedToGraphGroup = 6,
	LogEntry_l_workerMadeAvailable = 7,
	LogEntry_l_workerAssigned = 8,
	LogEntry_l_workerAssigning = 9,
	LogEntry_l_logReceived = 10,
	LogEntry_l_logFiled = 11,
	LogEntry_l_workerAssignmentDone = 12,
	LogEntry_l_pkgFetchStarting = 13,
	LogEntry_l_pkgFetchComplete = 14,
	LogEntry_l_pkgMarkedUnbuildable = 15
};

struct LogEntry {
	capnp_nowarn struct {
		uint64_t tvSec;
		uint64_t tvNSec;
	} time;
	enum LogEntry_l_which l_which;
	capnp_nowarn union {
		capnp_nowarn struct {
			PkgSpec_ptr pkg;
		} pkgImported;
		capnp_nowarn struct {
			PkgSpec_ptr pkg;
		} pkgAddedToGraph;
		capnp_nowarn struct {
			capn_text commitID;
		} graphSaved;
		capnp_nowarn struct {
			capn_text pkgname;
		} pkgImportedForDeletion;
		capnp_nowarn struct {
			capn_text commitID;
		} graphRead;
		capnp_nowarn struct {
			PkgSpec_list pkgs;
		} queueSelected;
		capnp_nowarn struct {
			Worker_ptr worker;
		} workerAddedToGraphGroup;
		capnp_nowarn struct {
			Worker_ptr worker;
		} workerMadeAvailable;
		capnp_nowarn struct {
			Worker_ptr worker;
			PkgSpec_ptr pkg;
		} workerAssigned;
		capnp_nowarn struct {
			Worker_ptr worker;
			PkgSpec_ptr pkg;
		} workerAssigning;
		capnp_nowarn struct {
			Worker_ptr worker;
			PkgSpec_ptr pkg;
		} logReceived;
		capnp_nowarn struct {
			PkgSpec_ptr pkg;
		} logFiled;
		capnp_nowarn struct {
			Worker_ptr worker;
			PkgSpec_ptr pkg;
			uint8_t cause;
		} workerAssignmentDone;
		capnp_nowarn struct {
			PkgSpec_ptr pkg;
		} pkgFetchStarting;
		capnp_nowarn struct {
			PkgSpec_ptr pkg;
		} pkgFetchComplete;
		capnp_nowarn struct {
			PkgSpec_ptr pkg;
		} pkgMarkedUnbuildable;
	} l;
};

static const size_t LogEntry_word_count = 3;

static const size_t LogEntry_pointer_count = 2;

static const size_t LogEntry_struct_bytes_count = 40;

struct PkgSpec {
	capn_text name;
	capn_text ver;
	enum Arch arch;
};

static const size_t PkgSpec_word_count = 1;

static const size_t PkgSpec_pointer_count = 2;

static const size_t PkgSpec_struct_bytes_count = 24;

capn_text PkgSpec_get_name(PkgSpec_ptr p);

capn_text PkgSpec_get_ver(PkgSpec_ptr p);

enum Arch PkgSpec_get_arch(PkgSpec_ptr p);

void PkgSpec_set_name(PkgSpec_ptr p, capn_text name);

void PkgSpec_set_ver(PkgSpec_ptr p, capn_text ver);

void PkgSpec_set_arch(PkgSpec_ptr p, enum Arch arch);

struct Worker {
	uint16_t addr;
	uint32_t check;
	enum Arch hostarch;
	enum Arch trgtarch;
	unsigned iscross : 1;
	uint16_t cost;
	unsigned isvalid : 1;
};

static const size_t Worker_word_count = 2;

static const size_t Worker_pointer_count = 0;

static const size_t Worker_struct_bytes_count = 16;

uint16_t Worker_get_addr(Worker_ptr p);

uint32_t Worker_get_check(Worker_ptr p);

enum Arch Worker_get_hostarch(Worker_ptr p);

enum Arch Worker_get_trgtarch(Worker_ptr p);

unsigned Worker_get_iscross(Worker_ptr p);

uint16_t Worker_get_cost(Worker_ptr p);

unsigned Worker_get_isvalid(Worker_ptr p);

void Worker_set_addr(Worker_ptr p, uint16_t addr);

void Worker_set_check(Worker_ptr p, uint32_t check);

void Worker_set_hostarch(Worker_ptr p, enum Arch hostarch);

void Worker_set_trgtarch(Worker_ptr p, enum Arch trgtarch);

void Worker_set_iscross(Worker_ptr p, unsigned iscross);

void Worker_set_cost(Worker_ptr p, uint16_t cost);

void Worker_set_isvalid(Worker_ptr p, unsigned isvalid);

LogEntry_ptr new_LogEntry(struct capn_segment*);
PkgSpec_ptr new_PkgSpec(struct capn_segment*);
Worker_ptr new_Worker(struct capn_segment*);

LogEntry_list new_LogEntry_list(struct capn_segment*, int len);
PkgSpec_list new_PkgSpec_list(struct capn_segment*, int len);
Worker_list new_Worker_list(struct capn_segment*, int len);

void read_LogEntry(struct LogEntry*, LogEntry_ptr);
void read_PkgSpec(struct PkgSpec*, PkgSpec_ptr);
void read_Worker(struct Worker*, Worker_ptr);

void write_LogEntry(const struct LogEntry*, LogEntry_ptr);
void write_PkgSpec(const struct PkgSpec*, PkgSpec_ptr);
void write_Worker(const struct Worker*, Worker_ptr);

void get_LogEntry(struct LogEntry*, LogEntry_list, int i);
void get_PkgSpec(struct PkgSpec*, PkgSpec_list, int i);
void get_Worker(struct Worker*, Worker_list, int i);

void set_LogEntry(const struct LogEntry*, LogEntry_list, int i);
void set_PkgSpec(const struct PkgSpec*, PkgSpec_list, int i);
void set_Worker(const struct Worker*, Worker_list, int i);

#ifdef __cplusplus
}
#endif
#endif
