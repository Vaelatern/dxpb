/*
 * btranslate.c
 *
 * Module for translating from one form of message to another.
 */
#define _POSIX_C_SOURCE 200809L

#include <czmq.h>
#include "btranslate.h"
#include "dxpb.h"

/*
 * Returns one of the enum values, but returns the largest possible enum value
 * as an error code
 */
enum translate_types
btranslate_type_of_msg(zframe_t *frame)
{
	enum translate_types curRet;
	if (zframe_size(frame) != sizeof(enum translate_types))
		return TRANSLATE_NUM_MAX;
	for (curRet = 0; curRet < TRANSLATE_NUM_MAX; curRet++)
		if (curRet == *(zframe_data(frame)))
			goto end;
end:
	return curRet;
}

zframe_t *
btranslate_prepare_frame(enum translate_types type)
{
	zframe_t *retFrame = NULL;
	if (type >= 0 && type < TRANSLATE_NUM_MAX) {
		retFrame = zframe_new(&type, sizeof(enum translate_types));
		zframe_set_more(retFrame, 1);
	}
	return retFrame;
}

int
btranslate_prepare_socket(zsock_t *sock, enum translate_types type)
{
	int rc;
	zframe_t *frame = btranslate_prepare_frame(type);
	if (!frame)
		return ERR_CODE_BAD;
	rc = zframe_send(&frame, sock, ZFRAME_MORE);
	if (rc != 0)
		return ERR_CODE_BAD;
	return ERR_CODE_OK;
}
