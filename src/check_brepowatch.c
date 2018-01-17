#define _POSIX_C_SOURCE 200809L

#include "brepowatch.c"
#include "check_main.inc"

#define MAKEFILE(x,y) { \
			char *newname = bstring_add(strdup(x), y, NULL, NULL); \
			bfs_touch(newname); \
			free(newname); \
			newname = NULL; \
		}

#define GETRESP(sock,x) { \
			char *str = zstr_recv(sock); \
			ck_assert_str_eq(str, x); \
			free(str); \
			}

START_TEST(test_filefinder_basic)
{
	char *tmppath = strdup("/tmp/dxpb-check_brepowatch-XXXXXX");
	char *ourdir = mkdtemp(tmppath);
	char *workdir = bstring_add(strdup(ourdir), "/work", NULL, NULL);
	char *sockdirpatt = bstring_add(strdup(ourdir), "/sock", NULL, NULL);
	ck_assert_ptr_nonnull(ourdir);
	ck_assert_ptr_nonnull(workdir);
	ck_assert_ptr_nonnull(sockdirpatt);
	mkdir(ourdir, S_IRWXU);
	mkdir(workdir, S_IRWXU);

	zsock_t *sock = NULL;
	pthread_t *thread = brepowatch_filefinder_prepare(&sock, workdir, sockdirpatt);
	ck_assert_ptr_nonnull(sock);

	zstr_send(sock, NULL);
	zsock_destroy(&sock);
	void *threadRet = NULL;
	pthread_join(*thread, &threadRet);
	free(thread); thread = NULL;
	ck_assert_ptr_null(threadRet);
	free(sockdirpatt);
	free(workdir);
	char *runcmd = bstring_add(strdup("rm -rf "), ourdir, NULL, NULL);
	(void)system(runcmd);
	free(ourdir);
	ourdir = NULL;
	free(runcmd);
	runcmd = NULL;
}
END_TEST

START_TEST(test_filefinder_static_files_one_dir)
{
	char *tmppath = strdup("/tmp/dxpb-check_brepowatch-XXXXXX");
	char *ourdir = mkdtemp(tmppath);
	char *workdir = bstring_add(strdup(ourdir), "/work", NULL, NULL);
	char *sockdirpatt = bstring_add(strdup(ourdir), "/sock", NULL, NULL);
	ck_assert_ptr_nonnull(ourdir);
	ck_assert_ptr_nonnull(workdir);
	ck_assert_ptr_nonnull(sockdirpatt);
	mkdir(ourdir, S_IRWXU);
	mkdir(workdir, S_IRWXU);

	MAKEFILE(workdir,"/1");
	MAKEFILE(workdir,"/abcdef");
	MAKEFILE(workdir,"/ghijkl");
	MAKEFILE(workdir,"/jkl");
	MAKEFILE(workdir,"/mno");
	MAKEFILE(workdir,"/pqr");
	MAKEFILE(workdir,"/stu");
	MAKEFILE(workdir,"/vwx");

	zsock_t *sock = NULL;
	pthread_t *thread = brepowatch_filefinder_prepare(&sock, workdir, sockdirpatt);
	ck_assert_ptr_nonnull(sock);

	zstr_send(sock, "1");
	GETRESP(sock, "/");
	zstr_send(sock, "abcdef");
	GETRESP(sock, "/");
	zstr_send(sock, "mno");
	GETRESP(sock, "/");
	zstr_send(sock, "vwx");
	GETRESP(sock, "/");
	zstr_send(sock, "not_there");
	GETRESP(sock, "");


	zstr_send(sock, NULL);
	zsock_destroy(&sock);
	void *threadRet = NULL;
	pthread_join(*thread, &threadRet);
	free(thread); thread = NULL;
	ck_assert_ptr_null(threadRet);
	free(sockdirpatt);
	free(workdir);
	char *runcmd = bstring_add(strdup("rm -rf "), ourdir, NULL, NULL);
	(void)system(runcmd);
	free(ourdir);
	ourdir = NULL;
	free(runcmd);
	runcmd = NULL;
}
END_TEST

START_TEST(test_filefinder_static_files_sub_dir)
{
	char *tmppath = strdup("/tmp/dxpb-check_brepowatch-XXXXXX");
	char *ourdir = mkdtemp(tmppath);
	char *workdir = bstring_add(strdup(ourdir), "/work", NULL, NULL);
	char *subdir1 = bstring_add(strdup(ourdir), "/work/mno", NULL, NULL);
	char *subdir2 = bstring_add(strdup(ourdir), "/work/mno/pqr", NULL, NULL);
	char *sockdirpatt = bstring_add(strdup(ourdir), "/sock", NULL, NULL);
	ck_assert_ptr_nonnull(ourdir);
	ck_assert_ptr_nonnull(workdir);
	ck_assert_ptr_nonnull(sockdirpatt);
	mkdir(ourdir, S_IRWXU);
	mkdir(workdir, S_IRWXU);
	mkdir(subdir1, S_IRWXU);
	mkdir(subdir2, S_IRWXU);

	MAKEFILE(workdir,"/1");
	MAKEFILE(workdir,"/abcdef");
	MAKEFILE(workdir,"/ghijkl");
	MAKEFILE(workdir,"/mno/pqr/tuv");
	MAKEFILE(workdir,"/mno/pqr/pqr");
	MAKEFILE(workdir,"/stu");
	MAKEFILE(workdir,"/vwx");

	zsock_t *sock = NULL;
	pthread_t *thread = brepowatch_filefinder_prepare(&sock, workdir, sockdirpatt);
	ck_assert_ptr_nonnull(sock);

	zstr_send(sock, "1");
	GETRESP(sock, "/");
	zstr_send(sock, "abcdef");
	GETRESP(sock, "/");
	zstr_send(sock, "mno");
	GETRESP(sock, "");
	zstr_send(sock, "tuv");
	GETRESP(sock, "/mno/pqr");
	zstr_send(sock, "pqr");
	GETRESP(sock, "/mno/pqr");
	zstr_send(sock, "vwx");
	GETRESP(sock, "/");
	zstr_send(sock, "not_there");
	GETRESP(sock, "");

	zstr_send(sock, NULL);
	zsock_destroy(&sock);
	void *threadRet = NULL;
	pthread_join(*thread, &threadRet);
	free(thread); thread = NULL;
	ck_assert_ptr_null(threadRet);
	free(subdir1);
	free(subdir2);
	free(sockdirpatt);
	free(workdir);
	char *runcmd = bstring_add(strdup("rm -rf "), ourdir, NULL, NULL);
	(void)system(runcmd);
	free(ourdir);
	ourdir = NULL;
	free(runcmd);
	runcmd = NULL;
}
END_TEST

START_TEST(test_filefinder_dynamic_files)
{
	char *tmppath = strdup("/tmp/dxpb-check_brepowatch-XXXXXX");
	char *ourdir = mkdtemp(tmppath);
	char *workdir = bstring_add(strdup(ourdir), "/work", NULL, NULL);
	char *sockdirpatt = bstring_add(strdup(ourdir), "/sock", NULL, NULL);
	ck_assert_ptr_nonnull(ourdir);
	ck_assert_ptr_nonnull(workdir);
	ck_assert_ptr_nonnull(sockdirpatt);
	mkdir(ourdir, S_IRWXU);
	mkdir(workdir, S_IRWXU);

	MAKEFILE(workdir,"/1");
	MAKEFILE(workdir,"/abcdef");
	MAKEFILE(workdir,"/ghijkl");
	MAKEFILE(workdir,"/jkl");
	MAKEFILE(workdir,"/mno");
	MAKEFILE(workdir,"/pqr");
	MAKEFILE(workdir,"/stu");
	MAKEFILE(workdir,"/vwx");

	zsock_t *sock = NULL;
	pthread_t *thread = brepowatch_filefinder_prepare(&sock, workdir, sockdirpatt);
	ck_assert_ptr_nonnull(sock);

	zstr_send(sock, "1");
	GETRESP(sock, "/");
	zstr_send(sock, "abcdef");
	GETRESP(sock, "/");
	zstr_send(sock, "mno");
	GETRESP(sock, "/");
	zstr_send(sock, "vwx");
	GETRESP(sock, "/");
	zstr_send(sock, "not_there");
	GETRESP(sock, "");
	zstr_send(sock, "is_there_now");
	GETRESP(sock, "");

	MAKEFILE(workdir,"/is_there_now");
	zstr_send(sock, "is_there_now");
	GETRESP(sock, "/");

	zstr_send(sock, "1");
	GETRESP(sock, "/");
	zstr_send(sock, "abcdef");
	GETRESP(sock, "/");

	zstr_send(sock, NULL);
	zsock_destroy(&sock);
	void *threadRet = NULL;
	pthread_join(*thread, &threadRet);
	free(thread); thread = NULL;
	ck_assert_ptr_null(threadRet);
	free(sockdirpatt);
	free(workdir);
	char *runcmd = bstring_add(strdup("rm -rf "), ourdir, NULL, NULL);
	(void)system(runcmd);
	free(ourdir);
	ourdir = NULL;
	free(runcmd);
	runcmd = NULL;
}
END_TEST

START_TEST(test_filefinder_dynamic_files_sub_dir)
{
	char *tmppath = strdup("/tmp/dxpb-check_brepowatch-XXXXXX");
	char *ourdir = mkdtemp(tmppath);
	char *workdir = bstring_add(strdup(ourdir), "/work", NULL, NULL);
	char *subdir1 = bstring_add(strdup(ourdir), "/work/mno", NULL, NULL);
	char *subdir2 = bstring_add(strdup(ourdir), "/work/mno/pqr", NULL, NULL);
	char *sockdirpatt = bstring_add(strdup(ourdir), "/sock", NULL, NULL);
	ck_assert_ptr_nonnull(ourdir);
	ck_assert_ptr_nonnull(workdir);
	ck_assert_ptr_nonnull(sockdirpatt);
	mkdir(ourdir, S_IRWXU);
	mkdir(workdir, S_IRWXU);
	mkdir(subdir1, S_IRWXU);
	mkdir(subdir2, S_IRWXU);

	MAKEFILE(workdir,"/1");
	MAKEFILE(workdir,"/abcdef");
	MAKEFILE(workdir,"/ghijkl");
	MAKEFILE(workdir,"/mno/pqr/tuv");
	MAKEFILE(workdir,"/mno/pqr/pqr");
	MAKEFILE(workdir,"/stu");
	MAKEFILE(workdir,"/vwx");

	zsock_t *sock = NULL;
	pthread_t *thread = brepowatch_filefinder_prepare(&sock, workdir, sockdirpatt);
	ck_assert_ptr_nonnull(sock);

	zstr_send(sock, "1");
	GETRESP(sock, "/");
	zstr_send(sock, "abcdef");
	GETRESP(sock, "/");
	zstr_send(sock, "mno");
	GETRESP(sock, "");
	zstr_send(sock, "tuv");
	GETRESP(sock, "/mno/pqr");
	zstr_send(sock, "pqr");
	GETRESP(sock, "/mno/pqr");
	zstr_send(sock, "vwx");
	GETRESP(sock, "/");
	zstr_send(sock, "not_there");
	GETRESP(sock, "");

	zstr_send(sock, "here_now");
	GETRESP(sock, "");
	MAKEFILE(workdir, "/mno/here_now");
	zstr_send(sock, "here_now");
	GETRESP(sock, "/mno");

	zstr_send(sock, "also_here_now");
	GETRESP(sock, "");
	MAKEFILE(workdir, "/mno/pqr/also_here_now");
	zstr_send(sock, "also_here_now");
	GETRESP(sock, "/mno/pqr");

	zstr_send(sock, NULL);
	zsock_destroy(&sock);
	void *threadRet = NULL;
	pthread_join(*thread, &threadRet);
	free(thread); thread = NULL;
	ck_assert_ptr_null(threadRet);
	free(subdir1);
	free(subdir2);
	free(sockdirpatt);
	free(workdir);
	char *runcmd = bstring_add(strdup("rm -rf "), ourdir, NULL, NULL);
	(void)system(runcmd);
	free(ourdir);
	ourdir = NULL;
	free(runcmd);
	runcmd = NULL;
}
END_TEST

Suite * test_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("BREPOWATCH");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_filefinder_basic);
	tcase_add_test(tc_core, test_filefinder_static_files_one_dir);
	tcase_add_test(tc_core, test_filefinder_static_files_sub_dir);
	tcase_add_test(tc_core, test_filefinder_dynamic_files);
	tcase_add_test(tc_core, test_filefinder_dynamic_files_sub_dir);
	suite_add_tcase(s, tc_core);

	return s;
}
