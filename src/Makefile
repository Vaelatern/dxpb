.POSIX:
.SUFFIXES: .c .o -help.txt -help.inc .xml

#########################################################
# Rules that we don't get because of .POSIX: at the top #
#########################################################

CC = cc
LINK.o = $(CC) $(LDFLAGS) $(TARGET_ARCH)
COMPILE.c = $(CC) $(LOCAL_CFLAGS) $(CFLAGS) $(CPPFLAGS) $(TARGET_ARCH) -c

.o:
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@

.c.o:
	$(COMPILE.c) $(OUTPUT_OPTION) $<


####################################################################
#  Layout: Vars, main, testing, actual rules, good rules to have   #
####################################################################

PREFIX ?= /usr/local

GSL_BIN ?= gsl

LOCAL_CFLAGS = -std=c11 -I../include/
CFLAGS ?= -g -Wall -Wextra -Wpedantic -Wno-missing-field-initializers -Werror -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2
LDFLAGS ?= -fPIC -fPIE
LDLIBS += -lzmq -lczmq

BINARIES = \
	dxpb-builder \
	dxpb-frontend \
	dxpb-grapher \
	dxpb-hostdir-master \
	dxpb-hostdir-remote \
	dxpb-pkgimport-agent \
	dxpb-pkgimport-master \
	dxpb-poke
TEST_BINARIES = check_bgraph check_bwords check_bstring check_btranslate check_bpkg check_bxbps
TEST_OBJECTS = check_bgraph.o check_bwords.o check_bstring.o check_btranslate.o check_bpkg.o check_bxbps.o
OBJECTS = \
	bbuilder.o \
	bdb.o \
	bfs.o \
	bgit.o \
	bgraph.o \
	bpkg.o \
	bstring.o \
	btranslate.o \
	bwords.o \
	bworker.o \
	bxbps.o \
	bxpkg.o \
	bxsrc.o \
	dxpb-builder.o \
	dxpb-common.o \
	dxpb-frontend.o \
	dxpb-grapher.o \
	dxpb-hostdir-master.o \
	dxpb-hostdir-remote.o \
	dxpb-pkgimport-agent.o \
	dxpb-pkgimport-master.o \
	dxpb-poke.o \
	pkgfiler.o \
	pkgfiler_grapher.o \
	pkgfiler_remote.o \
	pkgfiles_msg.o \
	pkggraph_filer.o \
	pkggraph_grapher.o \
	pkggraph_msg.o \
	pkggraph_server.o \
	pkggraph_worker.o \
	pkgimport_client.o \
	pkgimport_grapher.o \
	pkgimport_msg.o \
	pkgimport_poke.o \
	pkgimport_server.o

.PHONY: all main test check_%

main: prod

all: test

runnables: $(BINARIES)

license.inc: ../COPYING
	xxd -i $< $@

%-help.inc: ../doc/%-help.txt
	xxd -i $< $@

#############################
######  Testing targets #####
#############################

check_%: LDLIBS += -larchive -lxbps -lcheck

check_bpkg: check_bpkg.o pkgimport_msg.o bstring.o bxsrc.o bfs.o bxbps.o bwords.o bxpkg.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

check_bworker: check_bworker.o pkgimport_msg.o bwords.o bxsrc.o bstring.o bpkg.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

check_btranslate: check_btranslate.o pkgimport_msg.o pkggraph_msg.o pkgfiles_msg.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

check_bstring: check_bstring.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

check_bgraph: check_bgraph.o pkgimport_msg.o bstring.o bxbps.o bfs.o bxpkg.o bpkg.o bwords.o bxsrc.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

check_bwords: check_bwords.o bstring.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

check_bxbps: check_bxbps.o bwords.o bstring.o bfs.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

valgrind_check_%: check_%
	valgrind ./$^

testdb: LDLIBS += -lsqlite3 -larchive -lxbps
testdb: pkgimport_msg.o bxsrc.o bxbps.o bstring.o bwords.o bgraph.o bxpkg.o bdb.o


##################################
#### Rules for zproto things #####
##################################

.xml.c:
	$(GSL_BIN) -q -trace:1 $<
	touch $@

%_client.c: %_msg.c

%_server.c: %_msg.c

%_engine.inc: %.xml

##################################
#### Binaries and their needs ####
##################################

dxpb-poke: dxpb-poke.o pkgimport_poke.o pkgimport_msg.o

dxpb-pkgimport-agent: LDLIBS += -lbsd
dxpb-pkgimport-agent: dxpb-pkgimport-agent.o bxpkg.o bpkg.o bwords.o bstring.o bxsrc.o pkgimport_client.o pkgimport_msg.o

dxpb-pkgimport-master: LDLIBS += -lbsd -lgit2
dxpb-pkgimport-master: dxpb-pkgimport-master.o bxpkg.o bwords.o bstring.o bxsrc.o bfs.o bgit.o pkgimport_server.o pkgimport_msg.o

dxpb-builder: LDLIBS += -lgit2
dxpb-builder: dxpb-builder.o bstring.o bfs.o bgit.o bbuilder.o bwords.o bxpkg.o bxsrc.o pkggraph_worker.o pkggraph_msg.o

dxpb-grapher: LDLIBS += -lbsd -lsqlite3 -larchive -lxbps
dxpb-grapher: dxpb-grapher.o bstring.o bwords.o bxsrc.o bxpkg.o bpkg.o bfs.o bxbps.o bgraph.o bdb.o pkgimport_msg.o pkgimport_grapher.o pkggraph_msg.o pkggraph_grapher.o pkgfiles_msg.o pkgfiler_grapher.o bworker.o btranslate.o

dxpb-hostdir-master: LDLIBS += -larchive -lxbps
dxpb-hostdir-master: dxpb-hostdir-master.o bstring.o bfs.o bxbps.o pkggraph_msg.o pkggraph_filer.o pkgfiles_msg.o pkgfiler.o

dxpb-hostdir-remote: LDLIBS += -larchive -lxbps
dxpb-hostdir-remote: dxpb-hostdir-remote.o bxbps.o bstring.o bfs.o pkgfiles_msg.o pkgfiler_remote.o

dxpb-frontend: LDLIBS += -lbsd
dxpb-frontend: dxpb-frontend.o pkggraph_msg.o bxpkg.o pkggraph_server.o bworker.o

dxpb-common.c: license.inc

$(BINARIES): dxpb-common.o

########################
#### Generic Targets ###
########################

prod: $(OBJECTS) $(BINARIES)

test: $(TEST_BINARIES)

check: test

mostlyclean:
	rm -f $(OBJECTS)
	rm -f $(TEST_OBJECTS)
	rm -f $(TEST_BINARIES)

clean: mostlyclean
	rm -f $(BINARIES)

#############################
#### Installation Targets ###
#############################

install-default-dirs:
	mkdir -p $(DESTDIR)/etc/dxpb/curve/
	touch $(DESTDIR)/etc/dxpb/curve/.empty
	mkdir -p $(DESTDIR)/var/lib/dxpb/staging/
	touch $(DESTDIR)/var/lib/dxpb/staging/.empty
	mkdir -p $(DESTDIR)/var/lib/dxpb/logs/
	touch $(DESTDIR)/var/lib/dxpb/logs/.empty
	mkdir -p $(DESTDIR)/var/lib/dxpb/pkgs/
	touch $(DESTDIR)/var/lib/dxpb/pkgs/.empty
	mkdir -p $(DESTDIR)/var/run/dxpb/
	touch $(DESTDIR)/var/run/dxpb/.empty
	mkdir -p $(DESTDIR)/var/cache/dxpb/
	touch $(DESTDIR)/var/cache/dxpb/.empty

install-license:
	mkdir -p $(DESTDIR)/usr/share/licenses/dxpb/
	cp ../COPYING $(DESTDIR)/usr/share/licenses/dxpb/

install-doc:
	mkdir -p $(DESTDIR)/usr/share/doc/dxpb/
	cp -r ../doc $(DESTDIR)/usr/share/doc/dxpb/

install-bin: $(BINARIES)
	mkdir -p $(DESTDIR)/usr/bin/
	mv $(BINARIES) $(DESTDIR)/usr/bin/

install: install-default-dirs install-bin install-license


##################################
#             EOF                #
##################################
