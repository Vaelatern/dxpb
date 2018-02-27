.POSIX:

#########################################################
# Rules that we don't get because of .POSIX: at the top #
#########################################################

CC = clang
LINK.o = $(CC) $(LDFLAGS) $(TARGET_ARCH)
COMPILE.c = $(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

bin/%: obj/%.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

obj/%.o: src/%.c
	$(COMPILE.c) $< -o $@


####################################################################
#  Layout: Vars, main, testing, actual rules, good rules to have   #
####################################################################

PREFIX ?= /usr/local
BINDIR = $(PREFIX)/bin/
DOCDIR = $(PREFIX)/share/doc/dxpb/
MANDIR = $(PREFIX)/share/man/
SVDIR = /etc/sv/

GSL_BIN ?= gsl

LOCAL_CFLAGS = -std=c11 -Iinclude/
CFLAGS += -g -Wall -Wextra -Wpedantic -Wno-missing-field-initializers -Werror -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -finstrument-functions
LDFLAGS ?= -fPIC -fPIE
LDLIBS += -lzmq -lczmq

BINARIES = \
	bin/dxpb-builder \
	bin/dxpb-frontend \
	bin/dxpb-graph-to-dot \
	bin/dxpb-grapher \
	bin/dxpb-hostdir-master \
	bin/dxpb-hostdir-remote \
	bin/dxpb-pkgimport-agent \
	bin/dxpb-pkgimport-master \
	bin/dxpb-poke
SUID_BINARIES = \
	bin/dxpb-certs-remote \
	bin/dxpb-certs-server
HARNESSES = \
	testout/harness-grapher \
	testout/harness-hostdir-master \
	testout/harness-hostdir \
	testout/harness-builder \
	testout/harness-builder2 \
	testout/harness-frontend \
	testout/harness-pkgimport-master
HARNESS_OBJECTS = \
	obj/harness-grapher.o \
	obj/harness-hostdir-master.o \
	obj/harness-hostdir.o \
	obj/harness-builder.o \
	obj/harness-builder2.o \
	obj/harness-frontend.o \
	obj/harness-pkgimport-master.o
TEST_BINARIES = \
	testout/check_bgraph \
	testout/check_bwords \
	testout/check_bstring \
	testout/check_btranslate \
	testout/check_bpkg \
	testout/check_bxbps \
	testout/check_brepowatch \
	testout/usebfs \
	testout/usebgit
TEST_OBJECTS = \
	obj/check_bgraph.o \
	obj/check_bwords.o \
	obj/check_bstring.o \
	obj/check_btranslate.o \
	obj/check_bpkg.o \
	obj/check_bxbps.o \
	obj/check_brepowatch.o
OBJECTS = \
	obj/bbuilder.o \
	obj/bdb.o \
	obj/bdot.o \
	obj/bfs.o \
	obj/brepowatch.o \
	obj/bgit.o \
	obj/bgraph.o \
	obj/bpkg.o \
	obj/bstring.o \
	obj/btranslate.o \
	obj/bwords.o \
	obj/bworker.o \
	obj/bxbps.o \
	obj/bxpkg.o \
	obj/bxsrc.o \
	obj/dxpb-builder.o \
	obj/dxpb-certs-remote.o \
	obj/dxpb-certs-server.o \
	obj/dxpb-common.o \
	obj/dxpb-client.o \
	obj/dxpb-frontend.o \
	obj/dxpb-graph-to-dot.o \
	obj/dxpb-grapher.o \
	obj/dxpb-hostdir-master.o \
	obj/dxpb-hostdir-remote.o \
	obj/dxpb-pkgimport-agent.o \
	obj/dxpb-pkgimport-master.o \
	obj/dxpb-poke.o \
	obj/pkgfiler.o \
	obj/pkgfiler_grapher.o \
	obj/pkgfiler_remote.o \
	obj/pkgfiles_msg.o \
	obj/pkggraph_filer.o \
	obj/pkggraph_grapher.o \
	obj/pkggraph_msg.o \
	obj/pkggraph_server.o \
	obj/pkggraph_worker.o \
	obj/pkgimport_client.o \
	obj/pkgimport_grapher.o \
	obj/pkgimport_msg.o \
	obj/pkgimport_poke.o \
	obj/pkgimport_server.o \
	obj/ptrace.o
MYDIRS = bin obj testout

.PHONY: all main test check_% runnables prod clean mostlyclean harness

main: prod

all: test

release: README.md

runnables: $(MYDIRS) $(BINARIES) $(SUID_BINARIES) $(OBJECTS)

src/license.inc: COPYING
	xxd -i $< $@

src/%-help.inc: doc/%-help.txt
	xxd -i $< $@

src/dxpb-version.h: CHANGELOG.md
	awk '/## [0-9]+\.[0-9]+\.[0-9]+ -/ { print "#define DXPB_VERSION \"" $$2 "\""; exit }' CHANGELOG.md >src/dxpb-version.h

README.md: doc/dxpb.7
	mandoc -T markdown doc/dxpb.7 >README.md

#############################
######  Testing targets #####
#############################

testout/check_%: LDLIBS += -larchive -lxbps -lcheck

testout/check_bpkg: obj/check_bpkg.o obj/pkgimport_msg.o obj/bstring.o obj/bxsrc.o obj/bfs.o obj/bxbps.o obj/bwords.o obj/bxpkg.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

testout/check_bworker: obj/check_bworker.o obj/pkgimport_msg.o obj/bwords.o obj/bxsrc.o obj/bstring.o obj/bpkg.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

testout/check_btranslate: obj/check_btranslate.o obj/pkgimport_msg.o obj/pkggraph_msg.o obj/pkgfiles_msg.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

testout/check_bstring: obj/check_bstring.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

testout/check_bgraph: obj/check_bgraph.o obj/pkgimport_msg.o obj/bstring.o obj/bxbps.o obj/bfs.o obj/bxpkg.o obj/bpkg.o obj/bwords.o obj/bxsrc.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

testout/check_bwords: obj/check_bwords.o obj/bstring.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

testout/check_bxbps: obj/check_bxbps.o obj/bwords.o obj/bstring.o obj/bfs.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

testout/check_brepowatch: LDLIBS += -lpthread
testout/check_brepowatch: obj/check_brepowatch.o obj/bwords.o obj/bstring.o obj/bfs.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

testout/valgrind_check_%: testout/check_%
	valgrind -q --leak-check=full ./$^

testout/testdb: LDLIBS += -lsqlite3 -larchive -lxbps
testout/testdb: obj/pkgimport_msg.o obj/bxsrc.o obj/bxbps.o obj/bstring.o obj/bwords.o obj/bgraph.o obj/bxpkg.o obj/bdb.o


##################################
#### Rules for zproto things #####
##################################

src/%.xml: src/%.c
	$(GSL_BIN) -q -trace:1 $<
	touch $@

src/%_client.c: src/%_msg.c

src/%_server.c: src/%_msg.c

src/%_engine.inc: src/%.xml

##################################
#### Binaries and their needs ####
##################################

$(BINARIES): obj/bstring.o obj/bfs.o

bin/dxpb-poke: obj/dxpb-poke.o obj/pkgimport_poke.o obj/pkgimport_msg.o obj/dxpb-client.o

bin/dxpb-graph-to-dot: LDLIBS += -lsqlite3 -larchive -lxbps -lcgraph
bin/dxpb-graph-to-dot: obj/dxpb-graph-to-dot.o obj/bdb.o obj/bwords.o obj/bgraph.o obj/bxpkg.o obj/bpkg.o obj/bxsrc.o obj/pkgimport_msg.o obj/bxbps.o obj/bdot.o

bin/dxpb-pkgimport-agent: LDLIBS += -lbsd
bin/dxpb-pkgimport-agent: obj/dxpb-pkgimport-agent.o obj/dxpb-client.o obj/bxpkg.o obj/bpkg.o obj/bwords.o obj/bxsrc.o obj/pkgimport_client.o obj/pkgimport_msg.o

bin/dxpb-pkgimport-master: LDLIBS += -lbsd -lgit2
bin/dxpb-pkgimport-master: obj/dxpb-pkgimport-master.o obj/bxpkg.o obj/bwords.o obj/bxsrc.o obj/bgit.o obj/pkgimport_server.o obj/pkgimport_msg.o

bin/dxpb-builder: LDLIBS += -lgit2
bin/dxpb-builder: obj/dxpb-builder.o obj/dxpb-client.o obj/bgit.o obj/bbuilder.o obj/bwords.o obj/bxpkg.o obj/bxsrc.o obj/pkggraph_worker.o obj/pkggraph_msg.o

bin/dxpb-grapher: LDLIBS += -lbsd -lsqlite3 -larchive -lxbps
bin/dxpb-grapher: obj/dxpb-grapher.o obj/dxpb-client.o obj/bwords.o obj/bxsrc.o obj/bxpkg.o obj/bpkg.o obj/bxbps.o obj/bgraph.o obj/bdb.o obj/pkgimport_msg.o obj/pkgimport_grapher.o obj/pkggraph_msg.o obj/pkggraph_grapher.o obj/pkgfiles_msg.o obj/pkgfiler_grapher.o obj/bworker.o obj/btranslate.o

bin/dxpb-hostdir-master: LDLIBS += -larchive -lxbps -lpthread
bin/dxpb-hostdir-master: obj/dxpb-hostdir-master.o obj/brepowatch.o obj/bxbps.o obj/pkggraph_msg.o obj/pkggraph_filer.o obj/pkgfiles_msg.o obj/pkgfiler.o obj/dxpb-client.o

bin/dxpb-hostdir-remote: LDLIBS += -larchive -lxbps -lpthread
bin/dxpb-hostdir-remote: obj/dxpb-hostdir-remote.o obj/dxpb-client.o obj/bxbps.o obj/pkgfiles_msg.o obj/pkgfiler_remote.o obj/brepowatch.o

bin/dxpb-frontend: LDLIBS += -lbsd
bin/dxpb-frontend: obj/dxpb-frontend.o obj/pkggraph_msg.o obj/bxpkg.o obj/pkggraph_server.o obj/bworker.o

src/dxpb-common.c: src/license.inc src/dxpb-version.h

$(BINARIES): obj/dxpb-common.o obj/ptrace.o

$(SUID_BINARIES): obj/dxpb-common.o obj/bstring.o obj/bfs.o

$(OBJECTS): $(MYDIRS)

########################################
#### Test Harnesses and their needs ####
########################################

testout/harness-pkgimport-master: obj/harness-pkgimport-master.o obj/pkgimport_msg.o

testout/harness-grapher: obj/bxpkg.o obj/harness-grapher.o obj/pkgimport_msg.o obj/pkggraph_msg.o obj/pkgfiles_msg.o

testout/harness-hostdir-master: LDLIBS += -larchive -lxbps
testout/harness-hostdir-master: obj/bxbps.o obj/harness-hostdir-master.o obj/pkggraph_msg.o obj/pkgfiles_msg.o

testout/harness-hostdir: LDLIBS += -larchive -lxbps -lbsd
testout/harness-hostdir: obj/bxbps.o obj/harness-hostdir.o obj/pkggraph_msg.o obj/pkgfiles_msg.o

testout/harness-builder: obj/harness-builder.o obj/pkggraph_msg.o

testout/harness-builder2: obj/harness-builder2.o obj/pkggraph_msg.o

testout/harness-frontend: obj/harness-frontend.o obj/pkggraph_msg.o

$(HARNESSES): obj/dxpb-common.o obj/bfs.o obj/bstring.o obj/dxpb-client.o

harness: $(BINARIES) $(SUID_BINARIES) $(HARNESSES)

########################
#### Simple  Targets ###
########################

usebfs: bstring.o bfs.o

usebgit: LDLIBS += -lgit2
usebgit: bgit.o bwords.o bstring.o bxsrc.o bxpkg.o

########################
#### Generic Targets ###
########################

prod: runnables release $(MYDIRS)

$(MYDIRS):
	mkdir -p $@

test: $(TEST_BINARIES) prod $(HARNESSES)

check: test

mostlyclean:
	rm -f src/dxpb-version.h
	rm -f $(OBJECTS)
	rm -f $(TEST_OBJECTS)
	rm -f $(TEST_BINARIES)
	rm -f $(HARNESS_OBJECTS)
	rm -f $(HARNESSES)

clean: mostlyclean
	rm -f $(BINARIES)
	rm -f $(SUID_BINARIES)
	rmdir $(MYDIRS)

#############################
#### Installation Targets ###
#############################

install-license:
	mkdir -p $(DESTDIR)/usr/share/licenses/dxpb/
	cp COPYING $(DESTDIR)/usr/share/licenses/dxpb/

install-doc:
	mkdir -p $(DESTDIR)$(DOCDIR)
	cp -r ../doc $(DESTDIR)$(DOCDIR)

install-bin: $(SUID_BINARIES) $(BINARIES)
	mkdir -p $(DESTDIR)$(BINDIR)
	chmod +s $(SUID_BINARIES)
	mv $(SUID_BINARIES) $(BINARIES) $(DESTDIR)$(BINDIR)

install-runit-scripts: install-bin
	mkdir -p $(DESTDIR)$(SVDIR)
	cp -r ../runsv/* $(DESTDIR)$(SVDIR)

install-man:
	mkdir -p $(DESTDIR)$(MANDIR)/man7
	cp doc/dxpb.7 $(DESTDIR)$(MANDIR)/man7/

install: install-bin install-doc install-man install-license install-runit-scripts


##################################
#             EOF                #
##################################
