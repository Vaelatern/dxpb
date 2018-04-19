/*
 * bbuilder.h
 *
 */
#ifdef DXPB_BBUILDER_H
#error "Clean your dependencies"
#endif
#define DXPB_BBUILDER_H

enum bbuilder_actions {
	BBUILDER_QUIT,
	BBUILDER_STOP,
	BBUILDER_PING,
	BBUILDER_ROGER,
	BBUILDER_BOOTSTRAP,
	BBUILDER_BOOTSTRAP_DONE,
	BBUILDER_BUILD,
	BBUILDER_BUILDING,
	BBUILDER_GIVE_LOG,
	BBUILDER_LOG,
	BBUILDER_NOT_BUILDING,
	BBUILDER_NUM_MAX
};

int	bbuilder_agent(zsock_t *, char *, char *, char *);

extern const char *bbuilder_actions_picture[];

inline int
bbuilder_send(zsock_t *pipe, enum bbuilder_actions action, uint8_t val)
{
	zframe_t *frame = zframe_new(&action, sizeof(action));
	zframe_send(&frame, pipe, ZMQ_MORE);
	return zsock_bsend(pipe, bbuilder_actions_picture[action], &val);
}
