#include "log.capnp.h"
/* AUTO GENERATED - DO NOT EDIT */
#ifdef __GNUC__
# define capnp_unused __attribute__((unused))
# define capnp_use(x) (void) x;
#else
# define capnp_unused
# define capnp_use(x)
#endif

static const capn_text capn_val0 = {0,"",0};

LogEntry_ptr new_LogEntry(struct capn_segment *s) {
	LogEntry_ptr p;
	p.p = capn_new_struct(s, 24, 2);
	return p;
}
LogEntry_list new_LogEntry_list(struct capn_segment *s, int len) {
	LogEntry_list p;
	p.p = capn_new_list(s, len, 24, 2);
	return p;
}
void read_LogEntry(struct LogEntry *s capnp_unused, LogEntry_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->time.tvSec = capn_read64(p.p, 0);
	s->time.tvNSec = capn_read64(p.p, 8);
	s->l_which = (enum LogEntry_l_which)(int) capn_read16(p.p, 16);
	switch (s->l_which) {
	case LogEntry_l_pkgImported:
		s->l.pkgImported.pkg.p = capn_getp(p.p, 0, 0);
		break;
	case LogEntry_l_pkgAddedToGraph:
		s->l.pkgAddedToGraph.pkg.p = capn_getp(p.p, 0, 0);
		break;
	case LogEntry_l_graphSaved:
		s->l.graphSaved.commitID = capn_get_text(p.p, 0, capn_val0);
		break;
	case LogEntry_l_pkgImportedForDeletion:
		s->l.pkgImportedForDeletion.pkgname = capn_get_text(p.p, 0, capn_val0);
		break;
	case LogEntry_l_graphRead:
		s->l.graphRead.commitID = capn_get_text(p.p, 0, capn_val0);
		break;
	case LogEntry_l_queueSelected:
		s->l.queueSelected.pkgs.p = capn_getp(p.p, 0, 0);
		break;
	case LogEntry_l_workerAddedToGraphGroup:
		s->l.workerAddedToGraphGroup.worker.p = capn_getp(p.p, 0, 0);
		break;
	case LogEntry_l_workerMadeAvailable:
		s->l.workerMadeAvailable.worker.p = capn_getp(p.p, 0, 0);
		break;
	case LogEntry_l_workerAssigned:
		s->l.workerAssigned.worker.p = capn_getp(p.p, 0, 0);
		s->l.workerAssigned.pkg.p = capn_getp(p.p, 1, 0);
		break;
	case LogEntry_l_workerAssigning:
		s->l.workerAssigning.worker.p = capn_getp(p.p, 0, 0);
		s->l.workerAssigning.pkg.p = capn_getp(p.p, 1, 0);
		break;
	case LogEntry_l_logReceived:
		s->l.logReceived.worker.p = capn_getp(p.p, 0, 0);
		s->l.logReceived.pkg.p = capn_getp(p.p, 1, 0);
		break;
	case LogEntry_l_logFiled:
		s->l.logFiled.pkg.p = capn_getp(p.p, 0, 0);
		break;
	case LogEntry_l_workerAssignmentDone:
		s->l.workerAssignmentDone.worker.p = capn_getp(p.p, 0, 0);
		s->l.workerAssignmentDone.pkg.p = capn_getp(p.p, 1, 0);
		s->l.workerAssignmentDone.cause = capn_read8(p.p, 18);
		break;
	case LogEntry_l_pkgFetchStarting:
		s->l.pkgFetchStarting.pkg.p = capn_getp(p.p, 0, 0);
		break;
	case LogEntry_l_pkgFetchComplete:
		s->l.pkgFetchComplete.pkg.p = capn_getp(p.p, 0, 0);
		break;
	default:
		break;
	}
}
void write_LogEntry(const struct LogEntry *s capnp_unused, LogEntry_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write64(p.p, 0, s->time.tvSec);
	capn_write64(p.p, 8, s->time.tvNSec);
	capn_write16(p.p, 16, s->l_which);
	switch (s->l_which) {
	case LogEntry_l_pkgImported:
		capn_setp(p.p, 0, s->l.pkgImported.pkg.p);
		break;
	case LogEntry_l_pkgAddedToGraph:
		capn_setp(p.p, 0, s->l.pkgAddedToGraph.pkg.p);
		break;
	case LogEntry_l_graphSaved:
		capn_set_text(p.p, 0, s->l.graphSaved.commitID);
		break;
	case LogEntry_l_pkgImportedForDeletion:
		capn_set_text(p.p, 0, s->l.pkgImportedForDeletion.pkgname);
		break;
	case LogEntry_l_graphRead:
		capn_set_text(p.p, 0, s->l.graphRead.commitID);
		break;
	case LogEntry_l_queueSelected:
		capn_setp(p.p, 0, s->l.queueSelected.pkgs.p);
		break;
	case LogEntry_l_workerAddedToGraphGroup:
		capn_setp(p.p, 0, s->l.workerAddedToGraphGroup.worker.p);
		break;
	case LogEntry_l_workerMadeAvailable:
		capn_setp(p.p, 0, s->l.workerMadeAvailable.worker.p);
		break;
	case LogEntry_l_workerAssigned:
		capn_setp(p.p, 0, s->l.workerAssigned.worker.p);
		capn_setp(p.p, 1, s->l.workerAssigned.pkg.p);
		break;
	case LogEntry_l_workerAssigning:
		capn_setp(p.p, 0, s->l.workerAssigning.worker.p);
		capn_setp(p.p, 1, s->l.workerAssigning.pkg.p);
		break;
	case LogEntry_l_logReceived:
		capn_setp(p.p, 0, s->l.logReceived.worker.p);
		capn_setp(p.p, 1, s->l.logReceived.pkg.p);
		break;
	case LogEntry_l_logFiled:
		capn_setp(p.p, 0, s->l.logFiled.pkg.p);
		break;
	case LogEntry_l_workerAssignmentDone:
		capn_setp(p.p, 0, s->l.workerAssignmentDone.worker.p);
		capn_setp(p.p, 1, s->l.workerAssignmentDone.pkg.p);
		capn_write8(p.p, 18, s->l.workerAssignmentDone.cause);
		break;
	case LogEntry_l_pkgFetchStarting:
		capn_setp(p.p, 0, s->l.pkgFetchStarting.pkg.p);
		break;
	case LogEntry_l_pkgFetchComplete:
		capn_setp(p.p, 0, s->l.pkgFetchComplete.pkg.p);
		break;
	default:
		break;
	}
}
void get_LogEntry(struct LogEntry *s, LogEntry_list l, int i) {
	LogEntry_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_LogEntry(s, p);
}
void set_LogEntry(const struct LogEntry *s, LogEntry_list l, int i) {
	LogEntry_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_LogEntry(s, p);
}

PkgSpec_ptr new_PkgSpec(struct capn_segment *s) {
	PkgSpec_ptr p;
	p.p = capn_new_struct(s, 8, 2);
	return p;
}
PkgSpec_list new_PkgSpec_list(struct capn_segment *s, int len) {
	PkgSpec_list p;
	p.p = capn_new_list(s, len, 8, 2);
	return p;
}
void read_PkgSpec(struct PkgSpec *s capnp_unused, PkgSpec_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->name = capn_get_text(p.p, 0, capn_val0);
	s->ver = capn_get_text(p.p, 1, capn_val0);
	s->arch = (enum Arch)(int) capn_read16(p.p, 0);
}
void write_PkgSpec(const struct PkgSpec *s capnp_unused, PkgSpec_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_set_text(p.p, 0, s->name);
	capn_set_text(p.p, 1, s->ver);
	capn_write16(p.p, 0, (uint16_t) (s->arch));
}
void get_PkgSpec(struct PkgSpec *s, PkgSpec_list l, int i) {
	PkgSpec_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_PkgSpec(s, p);
}
void set_PkgSpec(const struct PkgSpec *s, PkgSpec_list l, int i) {
	PkgSpec_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_PkgSpec(s, p);
}

capn_text PkgSpec_get_name(PkgSpec_ptr p)
{
	capn_text name;
	name = capn_get_text(p.p, 0, capn_val0);
	return name;
}

capn_text PkgSpec_get_ver(PkgSpec_ptr p)
{
	capn_text ver;
	ver = capn_get_text(p.p, 1, capn_val0);
	return ver;
}

enum Arch PkgSpec_get_arch(PkgSpec_ptr p)
{
	enum Arch arch;
	arch = (enum Arch)(int) capn_read16(p.p, 0);
	return arch;
}

void PkgSpec_set_name(PkgSpec_ptr p, capn_text name)
{
	capn_set_text(p.p, 0, name);
}

void PkgSpec_set_ver(PkgSpec_ptr p, capn_text ver)
{
	capn_set_text(p.p, 1, ver);
}

void PkgSpec_set_arch(PkgSpec_ptr p, enum Arch arch)
{
	capn_write16(p.p, 0, (uint16_t) (arch));
}

Worker_ptr new_Worker(struct capn_segment *s) {
	Worker_ptr p;
	p.p = capn_new_struct(s, 16, 0);
	return p;
}
Worker_list new_Worker_list(struct capn_segment *s, int len) {
	Worker_list p;
	p.p = capn_new_list(s, len, 16, 0);
	return p;
}
void read_Worker(struct Worker *s capnp_unused, Worker_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	s->addr = capn_read16(p.p, 0);
	s->check = capn_read32(p.p, 4);
	s->hostarch = (enum Arch)(int) capn_read16(p.p, 2);
	s->trgtarch = (enum Arch)(int) capn_read16(p.p, 8);
	s->iscross = (capn_read8(p.p, 10) & 1) != 0;
	s->cost = capn_read16(p.p, 12);
	s->isvalid = (capn_read8(p.p, 10) & 2) != 0;
}
void write_Worker(const struct Worker *s capnp_unused, Worker_ptr p) {
	capn_resolve(&p.p);
	capnp_use(s);
	capn_write16(p.p, 0, s->addr);
	capn_write32(p.p, 4, s->check);
	capn_write16(p.p, 2, (uint16_t) (s->hostarch));
	capn_write16(p.p, 8, (uint16_t) (s->trgtarch));
	capn_write1(p.p, 80, s->iscross != 0);
	capn_write16(p.p, 12, s->cost);
	capn_write1(p.p, 81, s->isvalid != 0);
}
void get_Worker(struct Worker *s, Worker_list l, int i) {
	Worker_ptr p;
	p.p = capn_getp(l.p, i, 0);
	read_Worker(s, p);
}
void set_Worker(const struct Worker *s, Worker_list l, int i) {
	Worker_ptr p;
	p.p = capn_getp(l.p, i, 0);
	write_Worker(s, p);
}

uint16_t Worker_get_addr(Worker_ptr p)
{
	uint16_t addr;
	addr = capn_read16(p.p, 0);
	return addr;
}

uint32_t Worker_get_check(Worker_ptr p)
{
	uint32_t check;
	check = capn_read32(p.p, 4);
	return check;
}

enum Arch Worker_get_hostarch(Worker_ptr p)
{
	enum Arch hostarch;
	hostarch = (enum Arch)(int) capn_read16(p.p, 2);
	return hostarch;
}

enum Arch Worker_get_trgtarch(Worker_ptr p)
{
	enum Arch trgtarch;
	trgtarch = (enum Arch)(int) capn_read16(p.p, 8);
	return trgtarch;
}

unsigned Worker_get_iscross(Worker_ptr p)
{
	unsigned iscross;
	iscross = (capn_read8(p.p, 10) & 1) != 0;
	return iscross;
}

uint16_t Worker_get_cost(Worker_ptr p)
{
	uint16_t cost;
	cost = capn_read16(p.p, 12);
	return cost;
}

unsigned Worker_get_isvalid(Worker_ptr p)
{
	unsigned isvalid;
	isvalid = (capn_read8(p.p, 10) & 2) != 0;
	return isvalid;
}

void Worker_set_addr(Worker_ptr p, uint16_t addr)
{
	capn_write16(p.p, 0, addr);
}

void Worker_set_check(Worker_ptr p, uint32_t check)
{
	capn_write32(p.p, 4, check);
}

void Worker_set_hostarch(Worker_ptr p, enum Arch hostarch)
{
	capn_write16(p.p, 2, (uint16_t) (hostarch));
}

void Worker_set_trgtarch(Worker_ptr p, enum Arch trgtarch)
{
	capn_write16(p.p, 8, (uint16_t) (trgtarch));
}

void Worker_set_iscross(Worker_ptr p, unsigned iscross)
{
	capn_write1(p.p, 80, iscross != 0);
}

void Worker_set_cost(Worker_ptr p, uint16_t cost)
{
	capn_write16(p.p, 12, cost);
}

void Worker_set_isvalid(Worker_ptr p, unsigned isvalid)
{
	capn_write1(p.p, 81, isvalid != 0);
}
