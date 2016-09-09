#include "btranslate.c"
#include "check_main.c"

#include "pkgimport_msg.h"
#include "pkggraph_msg.h"
#include "pkgfiles_msg.h"

START_TEST(test_type_of_msg)
{
	zframe_t *frame;
	ck_assert_int_lt(sizeof(uint8_t), sizeof(enum translate_types));
	ck_assert_int_gt(sizeof(uint64_t), sizeof(enum translate_types));

	for (enum translate_types i = 0; i < TRANSLATE_NUM_MAX; i++) {
		frame = zframe_new(&i, sizeof(i));
		ck_assert_int_eq(btranslate_type_of_msg(frame), i);
		zframe_destroy(&frame);
	}
	{
		enum translate_types i = TRANSLATE_NUM_MAX+9;
		ck_assert_int_gt(i, TRANSLATE_NUM_MAX);
		frame = zframe_new(&i, sizeof(i));
		ck_assert_int_eq(btranslate_type_of_msg(frame), TRANSLATE_NUM_MAX);
		zframe_destroy(&frame);
	}
	{
		uint16_t i = 0;
		frame = zframe_new(&i, sizeof(i));
		ck_assert_int_eq(btranslate_type_of_msg(frame), TRANSLATE_NUM_MAX);
		zframe_destroy(&frame);
	}
	{
		uint8_t i = 0;
		frame = zframe_new(&i, sizeof(i));
		ck_assert_int_eq(btranslate_type_of_msg(frame), TRANSLATE_NUM_MAX);
		zframe_destroy(&frame);
	}

}
END_TEST

START_TEST(test_prepare_frame)
{
	zframe_t *frame = NULL;

	for(enum translate_types type = 0; type < TRANSLATE_NUM_MAX; type++) {
		frame = btranslate_prepare_frame(type);
		ck_assert_ptr_nonnull(frame);
		ck_assert_int_eq(zframe_size(frame), sizeof(enum translate_types));
		ck_assert_int_eq(*(zframe_data(frame)), type);
		ck_assert_int_eq(zframe_more(frame), 1);
		zframe_destroy(&frame);
	}
	frame = btranslate_prepare_frame(TRANSLATE_NUM_MAX);
	ck_assert_ptr_null(frame);

	frame = btranslate_prepare_frame(-1);
	ck_assert_ptr_null(frame);

	frame = btranslate_prepare_frame(TRANSLATE_NUM_MAX+5);
	ck_assert_ptr_null(frame);

}
END_TEST

START_TEST(test_send_msg)
{
	const char *endpoint = "inproc://test.run";
	zframe_t *frame;
	enum translate_types ttype;
	int rc;
	zsock_t *sender = zsock_new(ZMQ_PAIR);
	zsock_t *recver = zsock_new(ZMQ_PAIR);
	ck_assert_ptr_nonnull(sender);
	ck_assert_ptr_nonnull(recver);

	rc = zsock_bind(sender, endpoint);
	ck_assert_int_eq(rc, 0);
	ck_assert_str_eq(zsock_endpoint(sender), endpoint);
	rc = zsock_connect(recver, endpoint);
	ck_assert_int_eq(rc, 0);

	for (int i = 0; i < 2; i++) {
		pkgimport_msg_t *msg;
		frame = btranslate_prepare_frame(TRANSLATE_IMPORT);
		ck_assert_ptr_nonnull(frame);
		rc = zframe_send(&frame, sender, ZFRAME_MORE);
		ck_assert_int_eq(rc, 0);
		ck_assert_ptr_null(frame);
		msg = pkgimport_msg_new();
		pkgimport_msg_set_id(msg, PKGIMPORT_MSG_HELLO);
		pkgimport_msg_send(msg, sender);
	}

	for (int i = 0; i < 2; i++) {
		pkgimport_msg_t *msg = pkgimport_msg_new();
		frame = zframe_recv(recver);
		ttype = btranslate_type_of_msg(frame);
		ck_assert_int_eq(ttype, TRANSLATE_IMPORT);
		rc = pkgimport_msg_recv(msg, recver);
		ck_assert_int_eq(rc, 0);
		ck_assert_int_eq(pkgimport_msg_id(msg), PKGIMPORT_MSG_HELLO);
		pkgimport_msg_destroy(&msg);
		zsock_flush(recver); // allows us to assert multipart parts are
					// actually multipart instead of new
					// messages
	}

	for (int i = 0; i < 2; i++) {
		pkggraph_msg_t *msg;
		frame = btranslate_prepare_frame(TRANSLATE_GRAPH);
		ck_assert_ptr_nonnull(frame);
		rc = zframe_send(&frame, sender, ZFRAME_MORE);
		ck_assert_int_eq(rc, 0);
		ck_assert_ptr_null(frame);
		msg = pkggraph_msg_new();
		pkggraph_msg_set_id(msg, PKGGRAPH_MSG_HELLO);
		pkggraph_msg_send(msg, sender);
	}

	for (int i = 0; i < 2; i++) {
		pkggraph_msg_t *msg = pkggraph_msg_new();
		frame = zframe_recv(recver);
		ttype = btranslate_type_of_msg(frame);
		ck_assert_int_eq(ttype, TRANSLATE_GRAPH);
		rc = pkggraph_msg_recv(msg, recver);
		ck_assert_int_eq(rc, 0);
		ck_assert_int_eq(pkggraph_msg_id(msg), PKGGRAPH_MSG_HELLO);
		pkggraph_msg_destroy(&msg);
		zsock_flush(recver);
	}

	for (int i = 0; i < 2; i++) {
		pkgfiles_msg_t *msg;
		frame = btranslate_prepare_frame(TRANSLATE_FILES);
		ck_assert_ptr_nonnull(frame);
		rc = zframe_send(&frame, sender, ZFRAME_MORE);
		ck_assert_int_eq(rc, 0);
		ck_assert_ptr_null(frame);
		msg = pkgfiles_msg_new();
		pkgfiles_msg_set_id(msg, PKGFILES_MSG_HELLO);
		pkgfiles_msg_send(msg, sender);
	}

	for (int i = 0; i < 2; i++) {
		pkgfiles_msg_t *msg = pkgfiles_msg_new();
		frame = zframe_recv(recver);
		ttype = btranslate_type_of_msg(frame);
		ck_assert_int_eq(ttype, TRANSLATE_FILES);
		rc = pkgfiles_msg_recv(msg, recver);
		ck_assert_int_eq(rc, 0);
		ck_assert_int_eq(pkgfiles_msg_id(msg), PKGFILES_MSG_HELLO);
		pkgfiles_msg_destroy(&msg);
		zsock_flush(recver);
	}

	zsock_destroy(&sender);
	zsock_destroy(&recver);
}
END_TEST

START_TEST(test_send_msg_easy)
{
	const char *endpoint = "inproc://test.run.again";
	zframe_t *frame;
	enum translate_types ttype;
	int rc;
	zsock_t *sender = zsock_new(ZMQ_PAIR);
	zsock_t *recver = zsock_new(ZMQ_PAIR);
	ck_assert_ptr_nonnull(sender);
	ck_assert_ptr_nonnull(recver);

	rc = zsock_bind(sender, endpoint);
	ck_assert_int_eq(rc, 0);
	ck_assert_str_eq(zsock_endpoint(sender), endpoint);
	rc = zsock_connect(recver, endpoint);
	ck_assert_int_eq(rc, 0);

	for (int i = 0; i < 2; i++) {
		pkgimport_msg_t *msg;
		rc = btranslate_prepare_socket(sender, TRANSLATE_IMPORT);
		ck_assert_int_eq(rc, ERR_CODE_OK);
		msg = pkgimport_msg_new();
		pkgimport_msg_set_id(msg, PKGIMPORT_MSG_HELLO);
		pkgimport_msg_send(msg, sender);
	}

	for (int i = 0; i < 2; i++) {
		pkgimport_msg_t *msg = pkgimport_msg_new();
		frame = zframe_recv(recver);
		ttype = btranslate_type_of_msg(frame);
		ck_assert_int_eq(ttype, TRANSLATE_IMPORT);
		rc = pkgimport_msg_recv(msg, recver);
		ck_assert_int_eq(rc, 0);
		ck_assert_int_eq(pkgimport_msg_id(msg), PKGIMPORT_MSG_HELLO);
		pkgimport_msg_destroy(&msg);
		zsock_flush(recver); // allows us to assert multipart parts are
					// actually multipart instead of new
					// messages
	}

	for (int i = 0; i < 2; i++) {
		pkggraph_msg_t *msg;
		rc = btranslate_prepare_socket(sender, TRANSLATE_GRAPH);
		ck_assert_int_eq(rc, ERR_CODE_OK);
		msg = pkggraph_msg_new();
		pkggraph_msg_set_id(msg, PKGGRAPH_MSG_HELLO);
		pkggraph_msg_send(msg, sender);
	}

	for (int i = 0; i < 2; i++) {
		pkggraph_msg_t *msg = pkggraph_msg_new();
		frame = zframe_recv(recver);
		ttype = btranslate_type_of_msg(frame);
		ck_assert_int_eq(ttype, TRANSLATE_GRAPH);
		rc = pkggraph_msg_recv(msg, recver);
		ck_assert_int_eq(rc, 0);
		ck_assert_int_eq(pkggraph_msg_id(msg), PKGGRAPH_MSG_HELLO);
		pkggraph_msg_destroy(&msg);
		zsock_flush(recver);
	}

	for (int i = 0; i < 2; i++) {
		pkgfiles_msg_t *msg;
		rc = btranslate_prepare_socket(sender, TRANSLATE_FILES);
		ck_assert_int_eq(rc, ERR_CODE_OK);
		msg = pkgfiles_msg_new();
		pkgfiles_msg_set_id(msg, PKGFILES_MSG_HELLO);
		pkgfiles_msg_send(msg, sender);
	}

	for (int i = 0; i < 2; i++) {
		pkgfiles_msg_t *msg = pkgfiles_msg_new();
		frame = zframe_recv(recver);
		ttype = btranslate_type_of_msg(frame);
		ck_assert_int_eq(ttype, TRANSLATE_FILES);
		rc = pkgfiles_msg_recv(msg, recver);
		ck_assert_int_eq(rc, 0);
		ck_assert_int_eq(pkgfiles_msg_id(msg), PKGFILES_MSG_HELLO);
		pkgfiles_msg_destroy(&msg);
		zsock_flush(recver);
	}

	zsock_destroy(&sender);
	zsock_destroy(&recver);
}
END_TEST

Suite * test_suite(void)
{
	Suite *s;
	TCase *tc_core;

	s = suite_create("BTRANSLATE");

	/* Core test case */
	tc_core = tcase_create("Core");

	tcase_add_test(tc_core, test_type_of_msg);
	tcase_add_test(tc_core, test_prepare_frame);
	tcase_add_test(tc_core, test_send_msg);
	tcase_add_test(tc_core, test_send_msg_easy);
	suite_add_tcase(s, tc_core);

	return s;
}
