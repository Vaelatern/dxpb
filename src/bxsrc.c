/*
 * bxsrc.c
 *
 * Module for dealing with bxsrc instances, which are integer file descriptors
 * desribing one end of a set of pipes, the other side connected to a call to
 * xbps-src, used for reading calls.
 */
#define _POSIX_C_SOURCE 200809L

#include <poll.h>
#include <fcntl.h>
#include <assert.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/wait.h>
#include <bsd/unistd.h>
#include <czmq.h>
#include "dxpb.h"
#include "bwords.h"
#include "bxpkg.h"
#include "bstring.h"
#include "bxsrc.h"

/* Local prototypes */
static int	 set_fd_o_nonblock(int);
static char	*bxsrc_request_to_string(int);
static char	 bxsrc_did_print(int *);

static int
unset_fd_o_nonblock(int fd)
{
	assert(fd >= 0);
	int flags;
	errno = 0;
	if ((flags = fcntl(fd, F_GETFL)) < 0) {
		perror("Can't F_GETFL on this file descriptor");
		return -1;
	}
	if (flags & O_NONBLOCK)
		flags ^= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) == -1) {
		perror("Can't F_SETFL on this file descriptor");
		return -1;
	}
	return 0;
}

static int
set_fd_o_nonblock(int fd)
{
	assert(fd >= 0);
	int flags;
	errno = 0;
	if ((flags = fcntl(fd, F_GETFL)) < 0) {
		perror("Can't F_GETFL on this file descriptor");
		return -1;
	}
	flags |= O_NONBLOCK;
	if (fcntl(fd, F_SETFL, flags) == -1) {
		perror("Can't F_SETFL on this file descriptor");
		return -1;
	}
	return 0;
}

/* An xsrc_request is characterized two ways: it does not necessarily close the
 * pipe and it always has the sequence \n at the end, and not anywhere in the
 * middle.
 */
static char *
bxsrc_request_to_string(int fd)
{
	char *buf = NULL;
	uint32_t parA = 0;
	uint32_t parB = 0;

	char inbuf[80];
	int8_t inread = 0;
	enum fsm_state read_state = FSM_STATE_A;

	int pollret;
	struct pollfd pollfds[1] = {{ .fd = fd, .events = POLLIN }};

	do {
		pollret = poll(pollfds, 1, (30 * 1000)); /* 30 seconds ought be enough for anybody */
		if (!pollret) {
			fprintf(stderr, "Whups, I got bored of waiting for xbps-src to respond\n");
			return buf;
		}
		inread = read(fd, inbuf, 79);
		assert(inread <= 80);
		if (inread >= 0)
			inbuf[inread] = '\0';
		if (inread > 0 && inbuf[inread-1] == '\n') {
			inbuf[inread-1] = '\0';
			read_state = FSM_STATE_B;
		}
		if (inread == 0)
			sched_yield();

		/* Wash out unprintable characters. Good templates will never
		 * trigger this but people can be mean.
		 */
		for (uint8_t i = 0; i < inread-1; i++) /* -1 so don't catch \0 at end */
			if (inbuf[i] < 32)
				inbuf[i] = ' ';

		if (inread > 0)
			buf = bstring_add(buf, inbuf, &parA, &parB);
	} while (read_state != FSM_STATE_B);

	return buf;
}

zchunk_t *
bxsrc_get_log(int fds[], uint32_t max_size, int *logDone)
{
	assert(*logDone == 0);
	zchunk_t *retVal = zchunk_new(NULL, max_size);
	uint32_t size = 0;

	char inbuf[80];
	int8_t inread = 0;
	uint8_t insize = 79;

	do {
		if (max_size - insize <= size)
			insize = max_size - size;
		inread = read(fds[0], inbuf, insize);
		if (inread > 0)
			size = zchunk_extend(retVal, inbuf, inread);
	} while (inread > 0 && size < max_size);

	if (inread == 0) // EOF
		*logDone = 1;

	return retVal;
}

int
bxsrc_does_exist(const char *xbps_src, const char *pkg_name)
{
	assert(pkg_name[0]);
	char *const args[] = {(char *)xbps_src, "-q", "show", (char *)pkg_name, NULL};
	int retVal = -1;

	pid_t child;
	int fds[3], fds_read[2], fds_error[2];

	if (pipe(fds_read) == -1) {
		perror("Could not pipe()");
		exit(ERR_CODE_NOPIPE);
	}
	if (pipe(fds_error) == -1) {
		perror("Could not pipe()");
		exit(ERR_CODE_NOPIPE);
	}

	switch(child = fork()) {
	case -1:
		perror("Could not fork");
		exit(ERR_CODE_NOFORK);
		/* NOTREACHED */
	case 0: /* You are the child */
		/* Close all fds except our pipes to fd 0, 1, and 2, then
		 * clean up and carry on */
		if (fds_read[1] != 1) {
			dup2(fds_read[1], 1);
		}
		if (fds_error[1] != 1) {
			dup2(fds_error[1], 2);
		}
		close(fds_read[0]);
		close(fds_error[0]);
		execv(xbps_src, args);
		errno = 0;
		exit(ERR_CODE_OK);
	default: /* What a good grown up */
		fds[0] = fds_read[0];
		fds[2] = fds_error[0];
		set_fd_o_nonblock(fds[0]);
		set_fd_o_nonblock(fds[2]);
		/* Now clean up the toys you won't use */
		close(fds_read[1]);
		close(fds_error[1]);
		sched_yield(); /* It takes time for the child to mature */
		retVal = -1;
		while (retVal == -1) {
			if (bxsrc_did_print(fds))
				retVal = 1;
			if (bxsrc_did_err(fds))
				retVal = 0;
			sched_yield();
		}
		assert(fds[0] > 2 && fds[2] > 2);
		/* Wait for process to end */
		waitpid(child, NULL, 0);
		/* Close reading pipes */
		close(fds[0]);
		close(fds[2]);
		return retVal;
	}
}

static pid_t
bxsrc_init(const char *xbps_src, int *fds, char *const *args, char *env[], int merge_stderr)
{
	pid_t child;
	int fds_read[2], fds_write[2], fds_error[2];
	int rc, rcc;

	if (pipe(fds_read) == -1) {
		perror("Could not pipe()");
		exit(ERR_CODE_NOPIPE);
	}
	if (pipe(fds_write) == -1) {
		perror("Could not pipe()");
		exit(ERR_CODE_NOPIPE);
	}
	if (!merge_stderr && pipe(fds_error) == -1) {
		perror("Could not pipe()");
		exit(ERR_CODE_NOPIPE);
	}

	switch(child = fork()) {
	case -1:
		perror("Could not fork");
		exit(ERR_CODE_NOFORK);
		/* NOTREACHED */
	case 0: /* You are the child */
		/* Close all fds except our pipes to fd 0, 1, and 2, then
		 * clean up and carry on */
		rcc = dup2(fds_read[1], 1);
		assert(rcc == 1);
		close(fds_read[0]);

		dup2(fds_write[0], 0);
		close(fds_write[1]);

		if (merge_stderr)
			dup2(1, 2);
		else {
			dup2(fds_error[1], 2);
			close(fds_error[0]);
		}

		errno = 0;
		rcc = write(1, "GO", 2);
		assert(rcc == 2);
		rcc = execve(xbps_src, args, env);
		assert(rcc == -1);
		perror("Failed to spawn an xbps-src instance");
		exit(ERR_CODE_OK);
	default: /* What a good grown up */
		/* Now clean up the toys you won't use */
		fds[0] = fds_read[0];
		close(fds_read[1]);

		fds[1] = fds_write[1];
		set_fd_o_nonblock(fds[1]);
		close(fds_write[0]);

		if (!merge_stderr) {
			fds[2] = fds_error[0];
			set_fd_o_nonblock(fds[2]);
			close(fds_error[1]);
		}

		char buf[2];
		rc = read(fds[0], buf, 2);
		assert(rc == 2);
		assert(buf[0] == 'G' && buf[1] == 'O');

		set_fd_o_nonblock(fds[0]);
		sched_yield(); /* It takes time for the child to mature */
		return child;
	}
}

pid_t
bxsrc_init_build(const char *xbps_src, const char *pkg_name, int *fds,
		const char *masterdir, const char *hostdir,
		enum pkg_archs target_arch)
{
	assert(pkg_name[0]);
	char *const args_with_cross[] = {(char *)xbps_src, "-1",
		"-a", (char *)pkg_archs_str[target_arch], "-H", (char *)hostdir,
		"-m", (char *)masterdir, "pkg", (char *)pkg_name, NULL};
	char *const args_sans_cross[] = {(char *)xbps_src, "-1",
		"-H", (char *)hostdir, "-m", (char *)masterdir,
		"pkg", (char *)pkg_name, NULL};
	char xbps_arch[30];
	xbps_arch[0] = '\0';
	if (pkg_archs_str[target_arch] != NULL)
		snprintf(xbps_arch, 30, "XBPS_ARCH=%s", pkg_archs_str[target_arch]);
	char *env[] = {xbps_arch, NULL};
	char *const *args = (target_arch != ARCH_NUM_MAX ? args_with_cross : args_sans_cross);

	return bxsrc_init(xbps_src, fds, args, env, 1);
}

int
bxsrc_bootstrap_end(const int fds[], const pid_t c_pid)
{
	assert(fds[0] > 2 && fds[1] > 2);
	char buf[80];
	buf[79] = '\0';
	/* Close writing pipe */
	close(fds[1]);
	/* Wait for process to end */
	int retVal = 0;
	while (read(fds[0], buf, 79) > 0) {
		fprintf(stderr, "%s", buf);
		buf[0] = '\0';
	}
	waitpid(c_pid, &retVal, 0);
	/* Close reading pipe */
	unset_fd_o_nonblock(fds[0]);
	while (read(fds[0], buf, 79)) {
		fprintf(stderr, "%s", buf);
	}
	close(fds[0]);
	return retVal;
}

int
bxsrc_run_dumb_bootstrap(const char *xbps_src)
{
	int fds[3];
	fds[0] = -1;
	fds[1] = -1;
	fds[2] = -1;
	char *const args_bootstrap[] = {(char *)xbps_src, "binary-bootstrap", NULL};
	char *env[] = {NULL};

	pid_t c_pid = bxsrc_init(xbps_src, fds, args_bootstrap, env, 1);
	assert(fds[2] == -1);
	int rV = bxsrc_bootstrap_end(fds, c_pid);
	if (rV != 0)
		return rV;

	return rV;
}

int
bxsrc_run_bootstrap(const char *xbps_src, const char *masterdir,
		const char *host_arch, int iscross)
{
	int fds[3];
	fds[0] = -1;
	fds[1] = -1;
	fds[2] = -1;
	char *const args_bootstrap[] = {(char *)xbps_src,
			"-a", (char *)host_arch,
			 "-m", (char *)masterdir, "binary-bootstrap", NULL};
	char *const args_bootstrap_update_cross[] = {(char *)xbps_src,
			"-a", (char *)host_arch,
			 "-m", (char *)masterdir, "bootstrap-update", NULL};
	char *const args_bootstrap_update[] = {(char *)xbps_src,
			 "-m", (char *)masterdir, "bootstrap-update", NULL};
	char xbps_arch[30];
	snprintf(xbps_arch, 30, "XBPS_ARCH=%s", host_arch);
	char *env[] = {xbps_arch, NULL};

	pid_t c_pid = bxsrc_init(xbps_src, fds, args_bootstrap, env, 1);
	assert(fds[2] == -1);
	int rV = bxsrc_bootstrap_end(fds, c_pid);
	if (rV != 0)
		return rV;

	if (iscross)
		c_pid = bxsrc_init(xbps_src, fds, args_bootstrap_update_cross,
				env, 1);
	else
		c_pid = bxsrc_init(xbps_src, fds, args_bootstrap_update, env, 1);
	assert(fds[2] == -1);
	rV = bxsrc_bootstrap_end(fds, c_pid);
	return rV;
}

pid_t
bxsrc_init_read(const char *xbps_src, const char *pkg_name, int *fds,
		enum pkg_archs host_arch, enum pkg_archs target_arch)
{
	assert(pkg_name[0]);
	char *const args_with_cross[] = {(char *)xbps_src, "-a",
				(char *)pkg_archs_str[target_arch], "-q", "-i",
				"show-pkg-var", (char *)pkg_name, NULL};
	char *const args_sans_cross[] = {(char *)xbps_src, "-q", "-i",
				"show-pkg-var", (char *)pkg_name, NULL};
	char xbps_arch[30];
	snprintf(xbps_arch, 30, "XBPS_ARCH=%s", pkg_archs_str[host_arch]);
	char *env[] = {xbps_arch, NULL};
	char *const *args = (target_arch != ARCH_NUM_MAX ? args_with_cross : args_sans_cross);

	return bxsrc_init(xbps_src, fds, args, env, 0);
}

void
bxsrc_ask(int fd, const char *query)
{
	assert(fd && query[0]);
	size_t query_len = strlen(query);
	size_t rc = 0;
	char *new_query = malloc((query_len + 2) * sizeof(char));
	if (!new_query) {
		perror("Couldn't allocate query string");
		exit(ERR_CODE_NOMEM);
	}
	strncpy(new_query, query, query_len);
	*(new_query+query_len) = '\n';
	*(new_query+query_len+1) = '\0';

	assert(strlen(new_query) == query_len+1);
	rc = write(fd, new_query, query_len+1);
	assert(rc == query_len+1);
	free(new_query);
	sched_yield();
}

void
bxsrc_close(int fds[], pid_t c_pid)
{
	assert(fds[0] > 2 && fds[1] > 2 && fds[2] > 2);
	char buf[80];
	/* Close writing pipe */
	close(fds[1]);
	/* Wait for process to end */
	int subVal = 0;
	waitpid(c_pid, &subVal, 0);
	/* Close reading pipes */
	/* Read all reading pipes to the end */
	unset_fd_o_nonblock(fds[0]);
	unset_fd_o_nonblock(fds[2]);
	while (read(fds[0], buf, 79)) {
		;
	}
	while (read(fds[2], buf, 79)) {
		;
	}
	close(fds[0]);
	close(fds[2]);
	assert(!subVal);
}

int
bxsrc_build_end(const int fds[], const pid_t c_pid)
{
	assert(fds[0] > 2 && fds[1] > 2);
	char buf[80];
	/* Close writing pipe */
	close(fds[1]);
	/* Wait for process to end */
	int retVal = 0;
	if (waitpid(c_pid, &retVal, WNOHANG) == 0)
		kill(c_pid, SIGTERM);
	/* Close reading pipe */
	unset_fd_o_nonblock(fds[0]);
	while (read(fds[0], buf, 79)) {
		;
	}
	close(fds[0]);
	return retVal;
}

/* Returns 1 if it is set */
char
bxsrc_q_isset(int *fds, const char *query)
{
	assert(fds[0] && fds[1] && query[0]);
	bxsrc_ask(fds[1], query);
	char retVal;
	char *buf = bxsrc_request_to_string(fds[0]);

	assert(buf);
	if (buf[0] != '\0')
		retVal = 1;
	else
		retVal = 0;

	free(buf);
	return retVal;
}

struct bwords *
bxsrc_q_to_words(int *fds, const char *query)
{
	assert(fds[0] && fds[1] && query[0]);
	bxsrc_ask(fds[1], query);
	char *tmp = bxsrc_request_to_string(fds[0]);
	struct bwords *retVal = bwords_from_string(tmp, " ,\t\n");
	free(tmp);
	return retVal;
}

static char
bxsrc_did_print(int *fds)
{
	assert(fds[0]);
	char buffer[10];
	int status = read(fds[0], buffer, 9);
	return status != 0 && status != -1; /* Process closed pipe or no data */
}

char
bxsrc_did_err(int *fds)
{
	assert(fds[2]);
	char buffer[10];
	int status = read(fds[2], buffer, 9);
	return status != 0 && status != -1; /* Process closed pipe or no data */
}

bwords
bxsrc_query_pkgnames(const char *xbps_src, const char *pkg)
{
	int fds[3];
	pid_t c_pid = bxsrc_init_read(xbps_src, pkg, fds, ARCH_NOARCH, ARCH_NUM_MAX);
	bwords retWords = bxsrc_q_to_words(fds, "subpackages");
	bwords tmp = bxsrc_q_to_words(fds, "pkgname");
	if (bxsrc_did_err(fds)) {
		bwords_destroy(&retWords, 0);
		retWords = bwords_new();
		retWords = bwords_append_word(retWords, pkg, 0);
		bxsrc_close(fds, c_pid);
		return retWords;
	}
	bxsrc_close(fds, c_pid);
	if (!(tmp->words[0] != NULL && tmp->words[1] == NULL)) {
		perror("Bad package has many words in its pkgname");
		exit(ERR_CODE_BADPKG);
	}
	retWords = bwords_append_word(retWords, tmp->words[0], 1);
	bwords_destroy(&tmp, 0);
	return retWords;
}

char *
bxsrc_pkg_version(const char *xbps_src, const char *name)
{
	bwords tmp;
	char *version;
	int fds[3];
	pid_t c_pid = bxsrc_init_read(xbps_src, name, fds, ARCH_X86_64, ARCH_NUM_MAX);

	/* Get version_revision */
	tmp = bxsrc_q_to_words(fds, "version");
	if (!tmp || !(tmp->words) || !(tmp->words[0]) || tmp->words[1]) {
		fprintf(stderr, "Unable to get version from template!");
		exit(ERR_CODE_BADPKG);
	}
	version = strdup(tmp->words[0]);
	if (version == NULL) {
		perror("Unable to strdup a version");
		exit(ERR_CODE_NOMEM);
	}
	bwords_destroy(&tmp, 1);

	tmp = bxsrc_q_to_words(fds, "revision");
	assert(tmp->words[0] != NULL);
	assert(tmp->words[1] == NULL);
	version = bstring_add(bstring_add(version, "_", NULL, NULL), tmp->words[0], NULL, NULL);
	bwords_destroy(&tmp, 1);

	bxsrc_close(fds, c_pid);

	return version;
}
