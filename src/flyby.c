/*
 * dxpb.c
 *
 * This source file is depriciated.
 * It is kept around just in case I need some of the code from in here.
 */
#define _POSIX_C_SOURCE 200809L

#include <dirent.h> /* Directories in POSIX */
#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <bsd/unistd.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <poll.h> /* poll() */
#include <pthread.h> /* We use threads to try and cut down on total time */
#include <sched.h>
#include "dxpb.h"
#include <curl/curl.h>
#include <archive.h>
#include <archive_entry.h>
#include <plist/plist.h>
#include <sqlite3.h>

const char *TABLE_CREATE_STMT = "CREATE TABLE IF NOT EXISTS Packages \
( \
	name TEXT NOT NULL ON CONFLICT FAIL, \
	arch INTEGER NOT NULL ON CONFLICT FAIL, \
	hostneeds TEXT, \
	crosshostneeds TEXT, \
	destneeds TEXT, \
	crossdestneeds TEXT, \
	cancross BOOLEAN NOT NULL ON CONFLICT FAIL, \
	broken BOOLEAN NOT NULL ON CONFLICT FAIL, \
	version TEXT NOT NULL ON CONFLICT FAIL, \
	PRIMARY KEY (name, arch) \
) WITHOUT ROWID";

const char *ROW_CREATE_STMT = "INSERT OR REPLACE INTO Packages \
(name,version,arch,hostneeds,crosshostneeds,destneeds,crossdestneeds,cancross,broken) \
 VALUES ( \
	$name, \
	$version, \
	$arch, \
	$hostneeds, \
	$crosshostneeds, \
	$destneeds, \
	$crossdestneeds, \
	$cancross, \
	$broken \
)";

const char *global_repo_info[] = {"https://repo.voidlinux.eu/current/armv6l-repodata",
			"https://repo.voidlinux.eu/current/armv7l-repodata",
			"https://repo.voidlinux.eu/current/i686-repodata",
			"https://repo.voidlinux.eu/current/x86_64-repodata",
			"https://repo.voidlinux.eu/current/musl/armv6l-musl-repodata",
			"https://repo.voidlinux.eu/current/musl/armv7l-musl-repodata",
			"https://repo.voidlinux.eu/current/musl/x86_64-musl-repodata",
			"https://repo.voidlinux.eu/current/musl/nonfree/armv6l-musl-repodata",
			"https://repo.voidlinux.eu/current/musl/nonfree/armv7l-musl-repodata",
			"https://repo.voidlinux.eu/current/musl/nonfree/x86_64-musl-repodata",
			"https://repo.voidlinux.eu/current/multilib/x86_64-repodata",
			"https://repo.voidlinux.eu/current/multilib/nonfree/x86_64-repodata",
			"https://repo.voidlinux.eu/current/aarch64/aarch64-musl-repodata",
			"https://repo.voidlinux.eu/current/aarch64/aarch64-repodata",
			"https://repo.voidlinux.eu/current/aarch64/nonfree/aarch64-musl-repodata",
			"https://repo.voidlinux.eu/current/aarch64/nonfree/aarch64-repodata",
			NULL};

struct worker_thread_pack {
	void *context;
	struct arch *head;
};

void *
pkg_status_thread(void *context)
{
	errno = 0;
	assert(context);
	void *input = zmq_socket(context, ZMQ_PULL);
	void *stable = zmq_socket(context, ZMQ_REP);
	assert(input);
	assert(stable);
	int rc;
	rc = zmq_bind(input, "inproc://pkg_read_notice");
	assert(rc == 0);
	rc = zmq_bind(stable, "inproc://pkg_stable");
	assert(rc == 0);

	int64_t counter = 0;
	int64_t max_count = 0;
	enum fsm_state thread_state = FSM_STATE_A;
	char buf[80];
	zmq_pollitem_t poll_items[2] = {
		{ .socket = input, .events = ZMQ_POLLIN },
		{ .socket = stable, .events = ZMQ_POLLIN }
	};
	do {
		sched_yield();
		errno = 0;
		do {
			rc = zmq_poll(poll_items, 2, -1);
			if (errno != 0 && rc <= 0)
				perror("zmq_poll issue");
		} while (rc <= 0);
		assert(rc > 0);
		if (poll_items[0].revents & ZMQ_POLLIN) {
			rc = zmq_recv(input, buf, 79, ZMQ_DONTWAIT);
			assert(rc >= 0);
			buf[rc] = '\0';
			if (strcmp(buf, "REQQEDNEWPKG") == 0)
				max_count++;
			else if (strcmp(buf, "PROCESSEDPKG") == 0)
				counter++;
			else if (strcmp(buf, "GDDAYTHNKS") == 0)
				thread_state = FSM_STATE_C;
			else if (strcmp(buf, "RESET") == 0)
				counter = max_count = 0;
			else
				fprintf(stderr, "pkg_status_thread: Got malformed request on input socket\n");
			printf("\r%ld/%ld ", counter, max_count);
			fflush(stdout);
		}
		if (poll_items[1].revents & ZMQ_POLLIN) {
			rc = zmq_recv(stable, buf, 79, ZMQ_DONTWAIT);
			assert(rc >= 0);
			buf[rc] = '\0';
			if (strcmp(buf, "AREWESTABLE?") == 0)
				thread_state = FSM_STATE_B;
			else
				fprintf(stderr, "pkg_status_thread: Got malformed request on status socket\n");
		}
		if (thread_state == FSM_STATE_B && counter == max_count) {
			rc = zmq_send(stable, "YES", 3, 0);
			assert(rc == 3);
			thread_state = FSM_STATE_A;
		}
	} while (thread_state != FSM_STATE_C);
	zmq_close(input);
	zmq_close(stable);
	return 0;
}

void *
xbps_src_thread(void *input)
{
	struct worker_thread_pack *args = input;
	assert(args->context);
	assert(args->head);
	errno = 0;
	void *update_sock = zmq_socket(args->context, ZMQ_PUSH);
	void *sock = zmq_socket(args->context, ZMQ_PULL);
	if (errno) {
		fprintf(stderr, "errno = %d, %s\n", errno, zmq_strerror(errno));
		perror("Got an error with our socket");
	}
	assert(update_sock);
	assert(sock);
	int rc;
	int64_t more = 1; /* Used when interacting with {g,s}etsockopt */
	size_t more_len = sizeof(more); /* This too */
	rc = zmq_connect(update_sock, "inproc://pkg_read_notice");
	assert(rc == 0);
	rc = zmq_connect(sock, "inproc://xbps_src_thread");
	assert(rc == 0);
	char *tmp, *name;
	tmp = name = NULL;
	enum fsm_state msg_state = FSM_STATE_A;
	zmq_msg_t part;
	size_t part_size;
	size_t thread_num;
	do {
		rc = zmq_msg_init(&part);
		assert(rc == 0);
		/* Blocking call */
		errno = 0;
		rc = zmq_msg_recv(&part, sock, 0);
		if (errno != 0)
			perror("msg-recv issue");
		assert(rc != -1);
		rc = zmq_getsockopt(sock, ZMQ_RCVMORE, &more, &more_len);
		assert(rc == 0);
		part_size = zmq_msg_size(&part);
		if ((tmp = malloc(part_size+1)) == NULL) {
			perror("Couldn't allocate temporary string");
			exit(ERR_CODE_NOMEM);
		}
		memcpy(tmp, zmq_msg_data(&part), part_size);
		tmp[part_size] = '\0';
		zmq_msg_close(&part);
		switch(msg_state) {
		case FSM_STATE_A:
			if (strcmp(tmp, "PLZPROCTHIS") == 0) {
				assert(more);
				msg_state = FSM_STATE_B;
			} else if (strcmp(tmp, "GDDAYTHNKS") == 0) {
				assert(!more);
				msg_state = FSM_STATE_E;
			} else {
				msg_state = FSM_STATE_F;
			}
			free(tmp);
			break;
		case FSM_STATE_B:
			assert(more);
			thread_num = (size_t) *tmp;
			msg_state = FSM_STATE_C;
			break;
		case FSM_STATE_C:
			assert(tmp[0]);
			assert(strlen(tmp) == part_size);
			assert(!more);
			name = tmp;
			init_package(name, args->head, thread_num);
			zmq_send(update_sock, "PROCESSEDPKG", 12, 0);
			free(name);
			msg_state = FSM_STATE_A;
			break;
		case FSM_STATE_F: /* Waste time in here for non-cooperative messages */
			fprintf(stderr, "State machine: wasting time here!\n");
			if (!more)
				msg_state = FSM_STATE_A;
			free(tmp);
			break;
		default:
			fprintf(stderr, "Somehow FSM in thread got out of bounds\n");
			exit(ERR_CODE_BADDOBBY);
		}
	} while (msg_state != FSM_STATE_E);
	zmq_close(update_sock);
	zmq_close(sock);
	return 0;
}

void *
test_pkg_status_thread(void *context)
{
	errno = 0;
	assert(context);
	void *output = zmq_socket(context, ZMQ_PUSH);
	void *stable = zmq_socket(context, ZMQ_REQ);
	assert(output);
	assert(stable);
	int rc;
	rc = zmq_connect(output, "inproc://pkg_read_notice");
	assert(rc == 0);
	rc = zmq_connect(stable, "inproc://pkg_stable");
	assert(rc == 0);

	sched_yield();
	uint32_t i;
	char buf[80];
	for (i = 0; i < 10000; i++) {
		rc = zmq_send(output, "REQQEDNEWPKG", 12, 0);
		assert(rc == 12);
	}
	for (i = 0; i < 10000; i++) {
		rc = zmq_send(output, "PROCESSEDPKG", 12, 0);
		assert(rc == 12);
	}
	for (i = 0; i < 10000; i++) {
		rc = zmq_send(output, "REQQEDNEWPKG", 12, 0);
		assert(rc == 12);
		rc = zmq_send(output, "PROCESSEDPKG", 12, 0);
		assert(rc == 12);
	}
	for (i = 0; i < 10000; i++) {
		rc = zmq_send(output, "REQQEDNEWPKG", 12, 0);
		assert(rc == 12);
	}
	for (i = 0; i < 10000; i++) {
		rc = zmq_send(output, "PROCESSEDPKG", 12, 0);
		assert(rc == 12);
	}
	rc = zmq_send(stable, "AREWESTABLE?", 12, 0);
	assert(rc == 12);
	rc = zmq_recv(stable, buf, 79, 0);
	assert(rc >= 0);
	buf[rc] = '\0';
	assert(strcmp(buf, "YES") == 0);
	zmq_close(output);
	zmq_close(stable);
	return 0;
}

pthread_t
init_pkg_status_watcher(void *context, void **status_sock)
{
	*status_sock = zmq_socket(context, ZMQ_PUSH);
	int rc = zmq_connect(*status_sock, "inproc://pkg_read_notice");
	assert(rc == 0);
	pthread_t new_thread, tmp[2];
	pthread_create(&new_thread, NULL, pkg_status_thread, context);
	pthread_create(&tmp[0], NULL, test_pkg_status_thread, context);
	pthread_create(&tmp[1], NULL, test_pkg_status_thread, context);
	pthread_join(tmp[0], NULL);
	pthread_join(tmp[1], NULL);
	rc = zmq_send(*status_sock, "RESET", 5, 0);
	return new_thread;
}

void
destroy_pkg_status_watcher(pthread_t thread_id, void **status_sock)
{
	int rc;
	rc = zmq_send(*status_sock, "GDDAYTHNKS", 10, 0);
	assert(rc == 10);
	pthread_join(thread_id, NULL);
	zmq_close(*status_sock);
}

void
wait_until_pkg_status_clean(void *context)
{
	char buf[80];
	void *sock = zmq_socket(context, ZMQ_REQ);
	int rc = zmq_connect(sock, "inproc://pkg_stable");
	assert(rc == 0);
	do {
		rc = zmq_send(sock, "AREWESTABLE?", 12, 0);
		assert(rc != EFSM);
	} while (rc != 12);
	assert(rc == 12);
	rc = zmq_recv(sock, buf, 79, 0);
	assert(rc >= 0);
	buf[rc] = '\0';
	assert(strcmp(buf, "YES") == 0);
	zmq_close(sock);
	return;
}

pthread_t *
init_xbps_src_thread_pool(size_t num_threads, struct worker_thread_pack *args, void **send_sock)
{
	*send_sock = zmq_socket(args->context, ZMQ_PUSH);
	int rc = zmq_bind(*send_sock, "inproc://xbps_src_thread");
	assert(rc == 0);
	size_t thread_num;
	errno = 0;
	pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
	if (!threads) {
		perror("Couldn't allocate space to store threads");
		exit(ERR_CODE_NOMEM);
	}
	for (thread_num = 0; thread_num < num_threads; thread_num++) {
		pthread_create(threads+thread_num, NULL, xbps_src_thread, args);
	}
	return threads;
}

void
destroy_xbps_src_thread_pool(size_t num_threads, pthread_t *threads, void **send_sock)
{
	size_t thread_num;
	int rc;
	for (thread_num = 0; thread_num < num_threads; thread_num++) {
		rc = zmq_send(*send_sock, "GDDAYTHNKS", 10, 0);
		assert(rc == 10);
	}
	for (thread_num = 0; thread_num < num_threads; thread_num++) {
		pthread_join(threads[thread_num], NULL);
	}
	zmq_close(*send_sock);
}

void
init_arch_graph(struct arch **head)
{
	struct arch *tmp;
	for (enum pkg_archs i = ARCH_NOARCH; i < ARCH_HOST; i++) {
		if ((tmp = calloc(1, sizeof(struct arch))) == NULL) {
			perror("Could not allocate memory for arch structs");
			exit(ERR_CODE_NOMEM);
		}
		tmp->spec = i;
		pthread_mutex_init(&(tmp->pkgs_mutex), NULL);
		HASH_ADD(hh, *head, spec, sizeof(enum pkg_archs), tmp);
	}
}

void
init_libs()
{
	curl_global_init(CURL_GLOBAL_ALL);
}

void
retire_libs()
{
	curl_global_cleanup();
}

void
create_pkg_graph(struct arch *head)
{
	plist_t *repostatus = init_repo_plists(global_repo_info);
	enum pkg_archs curarch = ARCH_NOARCH;
}

void
server_thread()
{
	struct arch *head = NULL;
	void *z_context = zmq_ctx_new();
	assert(z_context);
	void *send_sock;
	void *status_sock;
	init_arch_graph(&head);
	size_t num_threads = HASH_COUNT(head) + 1;
	struct worker_thread_pack *args;
	if ((args = malloc(sizeof(struct worker_thread_pack))) == NULL) {
		perror("Couldn't allocate memory");
		exit(ERR_CODE_NOMEM);
	}
	args->context = z_context;
	args->head = head;
	pthread_t *thread_ids = init_xbps_src_thread_pool(num_threads, args, &send_sock);
	pthread_t watcher_thread = init_pkg_status_watcher(z_context, &status_sock);
	init_package_graph(num_threads, "/opt/dxpb/void-packages/srcpkgs/", send_sock, status_sock);
	wait_until_pkg_status_clean(z_context);
	update_packages_from_git();
	write_tree_to_db(head);
	destroy_xbps_src_thread_pool(num_threads, thread_ids, &send_sock);
	destroy_pkg_status_watcher(watcher_thread, &status_sock);
	zmq_ctx_destroy(z_context);
	return;
}

int
main(int argc, const char *argv[])
{
	init_libs();
	server_thread();
	retire_libs();
	return 0;
}
