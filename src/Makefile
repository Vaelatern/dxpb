.POSIX:
.SUFFIXES: .c .o -help.txt -help.inc .xml

#########################################################
# Rules that we don't get because of .POSIX: at the top #
#########################################################

CC = clang
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
BINDIR = $(PREFIX)/bin/
DOCDIR = $(PREFIX)/share/doc/dxpb/
SVDIR = /etc/sv/

GSL_BIN ?= gsl
CAPNP_BIN ?= capnp

LOCAL_CFLAGS = -std=c11 -I../include/
CFLAGS += -g -Wall -Wextra -Wpedantic -Wno-missing-field-initializers -Werror -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2
LDFLAGS ?= -fPIC -fPIE
LDLIBS += -lzmq -lczmq

BLOG_SET = blog.o bxpkg.o capnp/log.capnp.o
BINARIES = \
	dxpb-builder \
	dxpb-frontend \
	dxpb-graph-to-dot \
	dxpb-grapher \
	dxpb-hostdir-master \
	dxpb-hostdir-remote \
	dxpb-pkgimport-agent \
	dxpb-pkgimport-master \
	dxpb-poke
SUID_BINARIES = \
	dxpb-certs-remote \
	dxpb-certs-server
HARNESSES = harness-grapher \
	harness-hostdir-master \
	harness-hostdir \
	harness-hostdir2 \
	harness-builder \
	harness-builder2 \
	harness-frontend \
	harness-pkgimport-master
HARNESS_OBJECTS = harness-grapher.o \
	harness-hostdir-master.o \
	harness-hostdir.o \
	harness-hostdir2.o \
	harness-builder.o \
	harness-builder2.o \
	harness-frontend.o \
	harness-pkgimport-master.o
TEST_BINARIES = check_bworker check_bgraph check_bwords check_bstring check_btranslate check_bpkg check_bxbps check_brepowatch check_bdb usebfs usebgit useblog
TEST_OBJECTS = check_bworker.o check_bgraph.o check_bwords.o check_bstring.o check_btranslate.o check_bpkg.o check_bxbps.o check_brepowatch.o check_bdb.o
OBJECTS = \
	bbuilder.o \
	bdb.o \
	bdot.o \
	bfs.o \
	bgit.o \
	bgraph.o \
	blog.o \
	bpkg.o \
	brepowatch.o \
	bstring.o \
	btranslate.o \
	butil.o \
	bvirtpkg.o \
	bwords.o \
	bworker.o \
	bworkermatch.o \
	bxbps.o \
	bxpkg.o \
	bxsrc.o \
	capnp/log.capnp.o \
	dxpb-builder.o \
	dxpb-certs-remote.o \
	dxpb-certs-server.o \
	dxpb-client.o \
	dxpb-common.o \
	dxpb-frontend.o \
	dxpb-graph-to-dot.o \
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
	pkgimport_server.o \
	ptrace.o

.PHONY: all main test check_% release

main: release prod

all: test

release: ../README.md

runnables: $(BINARIES) $(SUID_BINARIES)

license.inc: ../COPYING
	xxd -i $< $@

%-help.inc: ../doc/%-help.txt
	xxd -i $< $@

dxpb-version.h: ../CHANGELOG.md
	awk '/## [0-9]+\.[0-9]+\.[0-9]+[\.0-9]* -/ { print "#define DXPB_VERSION \"" $$2 "\""; exit }' ../CHANGELOG.md >dxpb-version.h
dxpb-version.txt: ../CHANGELOG.md
	awk '/## [0-9]+\.[0-9]+\.[0-9]+[\.0-9]* -/ { print $$2; exit }' ../CHANGELOG.md >dxpb-version.txt
dxpb-tagline.txt: ../CHANGELOG.md
	awk '/## [0-9]+\.[0-9]+\.[0-9]+[\.0-9]* -/ { print $$0; exit }' ../CHANGELOG.md | cut -d\  -f6- >dxpb-tagline.txt
dxpb-tagline.h: dxpb-tagline.txt
	printf "#define DXPB_TAGLINE \"%s\"\n" "$$(cat dxpb-tagline.txt)" >dxpb-tagline.h

../README.md: ../doc/dxpb.7 ../FAQ.md
	echo "![Production ready-ish](https://img.shields.io/badge/%F0%9F%A4%B7-Production%20Ready%E2%80%90ish-691d78.svg)" >../README.md
	cat ../FAQ.md >> ../README.md
	printf "\n\n# Manual\n\n" >> ../README.md
	mandoc -T markdown ../doc/dxpb.7 >>../README.md

#############################
######  Testing targets #####
#############################

check_%: LDLIBS += -larchive -lxbps -lcheck

check_bpkg: check_bpkg.o pkgimport_msg.o bstring.o bxsrc.o bfs.o bxbps.o bwords.o bxpkg.o butil.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

check_bworker: LDLIBS+= -lbsd
check_bworker: check_bworker.o pkgimport_msg.o bwords.o bxsrc.o bstring.o bpkg.o bxpkg.o butil.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

check_btranslate: check_btranslate.o pkgimport_msg.o pkggraph_msg.o pkgfiles_msg.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

check_bstring: check_bstring.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

check_bgraph: check_bgraph.o pkgimport_msg.o bstring.o bxbps.o bfs.o bxpkg.o bpkg.o bwords.o bxsrc.o butil.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

check_bwords: check_bwords.o bstring.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

check_bxbps: check_bxbps.o bwords.o bstring.o bfs.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

check_bdb: LDLIBS+= -lsqlite3
check_bdb: check_bdb.o bwords.o bstring.o bxpkg.o bpkg.o bxsrc.o pkgimport_msg.o bgraph.o bxbps.o butil.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

check_brepowatch: LDLIBS += -lpthread
check_brepowatch: check_brepowatch.o bwords.o bstring.o bfs.o
	$(LINK.o) $^ $(LOADLIBES) $(LDLIBS) -o $@
	./$@

valgrind_check_%: check_%
	valgrind -q --leak-check=full ./$^

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
### Rules for capnproto things ###
##################################

capnp/log.capnp.o: capnp/log.capnp.c

capnp/log.capnp.c: log.capnp
	$(CAPNP_BIN) compile log.capnp -oc:capnp

blog.c: capnp/log.capnp.c

##################################
#### Binaries and their needs ####
##################################

$(BINARIES): bstring.o bfs.o butil.o

dxpb-poke: dxpb-poke.o pkgimport_poke.o pkgimport_msg.o dxpb-client.o

dxpb-graph-to-dot: LDLIBS += -lsqlite3 -larchive -lxbps -lcgraph
dxpb-graph-to-dot: dxpb-graph-to-dot.o bdb.o bwords.o bgraph.o bxpkg.o bpkg.o bxsrc.o pkgimport_msg.o bxbps.o bdot.o

dxpb-pkgimport-agent: LDLIBS += -lbsd
dxpb-pkgimport-agent: dxpb-pkgimport-agent.o dxpb-client.o bxpkg.o bpkg.o bwords.o bxsrc.o pkgimport_client.o pkgimport_msg.o

dxpb-pkgimport-master: LDLIBS += -lbsd -lgit2 -lcapnp_c
dxpb-pkgimport-master: dxpb-pkgimport-master.o bxpkg.o bwords.o bxsrc.o bgit.o pkgimport_server.o bvirtpkg.o pkgimport_msg.o $(BLOG_SET)

dxpb-builder: LDLIBS += -lgit2
dxpb-builder: dxpb-builder.o dxpb-client.o bgit.o bbuilder.o bwords.o bxpkg.o bxsrc.o pkggraph_worker.o pkggraph_msg.o

dxpb-grapher: LDLIBS += -lbsd -lsqlite3 -larchive -lxbps -lcapnp_c
dxpb-grapher: dxpb-grapher.o dxpb-client.o bwords.o bxsrc.o bxpkg.o bpkg.o bxbps.o bgraph.o bdb.o pkgimport_msg.o pkgimport_grapher.o pkggraph_msg.o pkggraph_grapher.o pkgfiles_msg.o pkgfiler_grapher.o bworker.o bworkermatch.o bvirtpkg.o btranslate.o $(BLOG_SET)

dxpb-hostdir-master: LDLIBS += -larchive -lxbps -lpthread -lcapnp_c
dxpb-hostdir-master: dxpb-hostdir-master.o brepowatch.o bxbps.o pkggraph_msg.o pkggraph_filer.o pkgfiles_msg.o pkgfiler.o dxpb-client.o $(BLOG_SET)

dxpb-hostdir-remote: LDLIBS += -larchive -lxbps -lpthread
dxpb-hostdir-remote: dxpb-hostdir-remote.o dxpb-client.o bxbps.o pkgfiles_msg.o pkgfiler_remote.o brepowatch.o

dxpb-frontend: LDLIBS += -lbsd -lcapnp_c
dxpb-frontend: dxpb-frontend.o pkggraph_msg.o bxpkg.o pkggraph_server.o bworker.o $(BLOG_SET)

dxpb-common.c: license.inc dxpb-version.h dxpb-tagline.h

$(BINARIES): dxpb-common.o

$(SUID_BINARIES): dxpb-common.o bstring.o bfs.o

########################################
#### Test Harnesses and their needs ####
########################################

harness-pkgimport-master: harness-pkgimport-master.o pkgimport_msg.o

harness-grapher: bxpkg.o harness-grapher.o pkgimport_msg.o pkggraph_msg.o pkgfiles_msg.o

harness-hostdir-master: LDLIBS += -larchive -lxbps
harness-hostdir-master: bxbps.o harness-hostdir-master.o pkggraph_msg.o pkgfiles_msg.o

harness-hostdir: LDLIBS += -larchive -lxbps -lbsd
harness-hostdir: bxbps.o harness-hostdir.o pkggraph_msg.o pkgfiles_msg.o

harness-hostdir2: LDLIBS += -larchive -lxbps -lbsd -lcrypto
harness-hostdir2: bxbps.o harness-hostdir2.o pkggraph_msg.o pkgfiles_msg.o

harness-builder: harness-builder.o pkggraph_msg.o

shim-builder: LDLIBS += -lpthread
shim-builder: bstring.o dxpb-common.o bfs.o dxpb-client.o shim-builder.o pkggraph_msg.o

harness-builder2: harness-builder2.o pkggraph_msg.o

harness-frontend: LDLIBS += -lbsd
harness-frontend: harness-frontend.o pkggraph_msg.o bxpkg.o

$(HARNESSES): dxpb-common.o bfs.o bstring.o dxpb-client.o

harness: $(BINARIES) $(SUID_BINARIES) $(HARNESSES)

########################
#### Simple  Targets ###
########################

usebfs: bstring.o bfs.o

usebgit: LDLIBS += -lgit2
usebgit: bgit.o bwords.o bstring.o bxsrc.o bxpkg.o

useblog: LDLIBS += -lcapnp_c
useblog: useblog.o bstring.o bxpkg.o capnp/log.capnp.o

########################
#### Generic Targets ###
########################

prod: $(OBJECTS) $(BINARIES) $(SUID_BINARIES)

test: $(TEST_BINARIES) $(HARNESSES)

check: test

mostlyclean:
	rm -f dxpb-version.h dxpb-version.txt dxpb-tagline.h dxpb-tagline.txt
	rm -f $(OBJECTS)
	rm -f $(TEST_OBJECTS)
	rm -f $(TEST_BINARIES)
	rm -f $(HARNESS_OBJECTS)
	rm -f $(HARNESSES)

clean: mostlyclean
	rm -f $(BINARIES)
	rm -f $(SUID_BINARIES)

#############################
#### Installation Targets ###
#############################

install-license:
	mkdir -p $(DESTDIR)/usr/share/licenses/dxpb/
	cp ../COPYING $(DESTDIR)/usr/share/licenses/dxpb/

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

install: install-bin install-doc install-license install-runit-scripts

#############################
#### Maintenance  Targets ###
#############################

gitrelease: ../README.md ../CHANGELOG.md dxpb-version.txt dxpb-tagline.txt
	git commit -m "$$(cat dxpb-version.txt)" ../
	git tag -a $$(cat dxpb-version.txt) -m "$$(cat dxpb-tagline.txt)"

##################################
#             EOF                #
##################################
