/*
 * blog.c
 *
 * Module for writing log lines
 */
#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <czmq.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "dxpb.h"
#include "bwords.h"
#include "bxpkg.h"
#include "bworker.h"
#include "dxpb-common.h"
#include "capnp/log.capnp.h"
#include "blog.h"

inline static enum Arch
pkg_archs_translate(const enum pkg_archs in)
{
	switch (in) {
	case ARCH_NOARCH: return Arch_noarch;
	case ARCH_AARCH64: return Arch_aarch64;
	case ARCH_AARCH64_MUSL: return Arch_aarch64Musl;
	case ARCH_ARMV5TEL: return Arch_armv5Tel;
	case ARCH_ARMV5TEL_MUSL: return Arch_armv5TelMusl;
	case ARCH_ARMV6L: return Arch_armv6l;
	case ARCH_ARMV6L_MUSL: return Arch_armv6lMusl;
	case ARCH_ARMV7L: return Arch_armv7l;
	case ARCH_ARMV7L_MUSL: return Arch_armv7lMusl;
	case ARCH_I686: return Arch_i686;
	case ARCH_I686_MUSL: return Arch_i686Musl;
	case ARCH_MIPSEL_MUSL: return Arch_mipselMusl;
	case ARCH_X86_64: return Arch_x8664;
	case ARCH_X86_64_MUSL: return Arch_x8664Musl;
	default: return Arch_virtual;
	}
}

char
blog_logging_on(const char in)
{
	static char on = 0;
	if (in < 0)
		return on;
	on = in;
	return -1;
}

char *
blog_logfile(const char *path)
{
	static char *stored = NULL;
	if (path == NULL) {
		if (stored == NULL)
			return DEFAULT_LOGFILE;
		return stored;
	}

	FREE(stored);
	stored = strdup(path);
	if (stored == NULL)
		exit(ERR_CODE_NOMEM);
	return NULL;
}

static void
blog_write_to_file(struct capn *c)
{
	int fd = open(blog_logfile(NULL),
			O_WRONLY | O_APPEND | O_CREAT | O_SYNC, 0664);
	assert(fd >= 0);
	int rc = capn_write_fd(c, write, fd, 1);
	assert(rc > 0);
	close(fd);
}

static void
blog_set_time(struct LogEntry *relptr)
{
	struct timespec tp = {0};
	int rc = clock_gettime(CLOCK_REALTIME, &tp);
	assert(rc == 0);
	relptr->time.tvSec = (uint64_t) tp.tv_sec;
	relptr->time.tvNSec = (uint64_t) tp.tv_nsec;
}

static void
blog_init_logentry(struct LogEntry *relptr, enum LogEntry_l_which which)
{
	relptr->l_which = which;
	blog_set_time(relptr);
}

static void
blog_text_from_chars(capn_text *txt, const char *in)
{
	txt->str = in;
	txt->len = strlen(in);
	txt->seg = NULL;
}

static void
blog_worker_set(struct capn_segment *cs, Worker_ptr *ptr, const uint16_t addr,
		const uint32_t check, const enum pkg_archs hostarch,
		const enum pkg_archs trgtarch, const char iscross,
		const uint16_t cost)
{
	struct Worker lamb = {
		.addr = addr,
		.check = check,
		.hostarch = pkg_archs_translate(hostarch),
		.trgtarch = pkg_archs_translate(trgtarch),
		.iscross = iscross,
		.cost = cost};

	*ptr = new_Worker(cs);
	write_Worker(&lamb, *ptr);
}

static void
blog_pkgspec_set_all(struct capn_segment *cs, PkgSpec_list *list,
		zlist_t *pkgs)
{
	struct PkgSpec spec;
	struct pkg *in;
	int i = 0;

	*list = new_PkgSpec_list(cs, zlist_size(pkgs));

	for (in = zlist_first(pkgs), i = 0; in; in = zlist_next(pkgs), i++) {
		blog_text_from_chars(&spec.name, in->name);
		blog_text_from_chars(&spec.ver, in->ver);
		spec.arch = pkg_archs_translate(in->arch);
		set_PkgSpec(&spec, *list, i);
	}

}

static void
blog_pkgspec_set(struct capn_segment *cs, PkgSpec_ptr *ptr, const char *name, const char *ver, const enum pkg_archs arch)
{
	struct PkgSpec lamb = {0};
	lamb.arch = pkg_archs_translate(arch);

	blog_text_from_chars(&lamb.name, name);
	blog_text_from_chars(&lamb.ver, ver);

	*ptr = new_PkgSpec(cs);
	write_PkgSpec(&lamb, *ptr);
}

void
blog_pkgImported(const char *name, const char *ver, const enum pkg_archs arch)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_pkgImported);
	blog_pkgspec_set(cs, &relptr.l.pkgImported.pkg, name, ver, arch);

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}

void
blog_pkgAddedToGraph(const char *name, const char *ver, const enum pkg_archs arch)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_pkgAddedToGraph);
	blog_pkgspec_set(cs, &relptr.l.pkgAddedToGraph.pkg, name, ver, arch);

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}

void
blog_pkgFetchStarting(const char *name, const char *ver, const enum pkg_archs arch)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_pkgFetchStarting);
	blog_pkgspec_set(cs, &relptr.l.pkgFetchStarting.pkg, name, ver, arch);

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}

void
blog_pkgFetchComplete(const char *name, const char *ver, const enum pkg_archs arch)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_pkgFetchComplete);
	blog_pkgspec_set(cs, &relptr.l.pkgFetchComplete.pkg, name, ver, arch);

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}

void
blog_graphSaved(const char *commitID)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_graphSaved);
	blog_text_from_chars(&relptr.l.graphSaved.commitID, commitID);

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}

void
blog_graphRead(const char *commitID)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_graphRead);
	blog_text_from_chars(&relptr.l.graphRead.commitID, commitID);

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}

void
blog_pkgImportedForDeletion(const char *pkgname)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_pkgImportedForDeletion);
	blog_text_from_chars(&relptr.l.pkgImportedForDeletion.pkgname, pkgname);

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}

void
blog_logFiled(const char *name, const char *ver, const enum pkg_archs arch)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_logFiled);
	blog_pkgspec_set(cs, &relptr.l.logFiled.pkg, name, ver, arch);

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}

void
blog_workerAddedToGraphGroup(const struct bworker *wrkr)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_workerAddedToGraphGroup);
	blog_worker_set(cs, &relptr.l.workerAddedToGraphGroup.worker, wrkr->myaddr,
			wrkr->mycheck, wrkr->hostarch, wrkr->arch,
			wrkr->iscross, wrkr->cost);

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}

void
blog_workerMadeAvailable(const struct bworker *wrkr)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_workerMadeAvailable);
	blog_worker_set(cs, &relptr.l.workerMadeAvailable.worker, wrkr->myaddr,
			wrkr->mycheck, wrkr->hostarch, wrkr->arch,
			wrkr->iscross, wrkr->cost);

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}

void
blog_workerAssigned(const struct bworker *wrkr, const char *name, const char *ver, const enum pkg_archs arch)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_workerAssigned);
	blog_pkgspec_set(cs, &relptr.l.workerAssigned.pkg, name, ver, arch);
	blog_worker_set(cs, &relptr.l.workerAssigned.worker, wrkr->myaddr,
			wrkr->mycheck, wrkr->hostarch, wrkr->arch,
			wrkr->iscross, wrkr->cost);

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}

void
blog_workerAssigning(const struct bworker *wrkr, const char *name, const char *ver, const enum pkg_archs arch)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_workerAssigning);
	blog_pkgspec_set(cs, &relptr.l.workerAssigning.pkg, name, ver, arch);
	blog_worker_set(cs, &relptr.l.workerAssigning.worker, wrkr->myaddr,
			wrkr->mycheck, wrkr->hostarch, wrkr->arch,
			wrkr->iscross, wrkr->cost);

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}

void
blog_logReceived(const struct bworker *wrkr, const char *name, const char *ver, const enum pkg_archs arch)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_logReceived);
	blog_pkgspec_set(cs, &relptr.l.logReceived.pkg, name, ver, arch);
	if (wrkr)
		blog_worker_set(cs, &relptr.l.logReceived.worker, wrkr->myaddr,
				wrkr->mycheck, wrkr->hostarch, wrkr->arch,
				wrkr->iscross, wrkr->cost);

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}

void
blog_workerAssignmentDone(const struct bworker *wrkr, const char *name,
		const char *ver, const enum pkg_archs arch, const uint8_t cause)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_workerAssignmentDone);
	blog_pkgspec_set(cs, &relptr.l.workerAssignmentDone.pkg, name, ver, arch);
	blog_worker_set(cs, &relptr.l.workerAssignmentDone.worker, wrkr->myaddr,
			wrkr->mycheck, wrkr->hostarch, wrkr->arch,
			wrkr->iscross, wrkr->cost);
	relptr.l.workerAssignmentDone.cause = cause;

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}

void
blog_queueSelected(zlist_t *next_for_arch)
{
	if (!blog_logging_on(-1))
		return;
	struct capn c;
	capn_init_malloc(&c);
	capn_ptr cr = capn_root(&c);
	struct capn_segment *cs = cr.seg;
	LogEntry_ptr ptr = new_LogEntry(cs);
	struct LogEntry relptr;

	blog_init_logentry(&relptr, LogEntry_l_queueSelected);
	blog_pkgspec_set_all(cs, &relptr.l.queueSelected.pkgs, next_for_arch);

	write_LogEntry(&relptr, ptr);
	int rc = capn_setp(capn_root(&c), 0, ptr.p);
	assert(rc == 0);
	blog_write_to_file(&c);
	capn_free(&c);
}
