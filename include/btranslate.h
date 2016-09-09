/*
 * btranslate.h
 *
 * Module for sending multiple message types over a single message pipe and
 * then being able to handle each type separately.
 *
 * The core concept is as follows:
 * ZeroMQ Messages use a frame oriented mechanism, primarily for routing
 * information. Each frame, [Addr] has an address, then there is a null frame
 * [] which is cut off by a ROUTER socket on recv, and appended by a DEALER on
 * transmit. Then after the null frame you have [Body] frames.
 *
 * We hijack these address frames where possible with a single integer value as
 * enumerated in the translate_types enum as a form of routing ID.
 *
 * THIS DOES NOT WORK WITH THE ZPROTO MSG GENERATORS, WHEN SENDING TO A ROUTER
 * SOCKET. But it will work with them when sending to a Dealer or some socket
 * that strips the [Addr] frame for you.
 */

enum translate_types {
	TRANSLATE_IMPORT,
	TRANSLATE_FILES,
	TRANSLATE_GRAPH,
	TRANSLATE_NUM_MAX
};

enum translate_types	btranslate_type_of_msg(zframe_t *);
zframe_t *	btranslate_prepare_frame(enum translate_types);
int	btranslate_prepare_socket(zsock_t *, enum translate_types);
