/*
 * bworker_end_status.h
 *
 * For telling the world why/if we failed.
 */
#ifdef DXPB_BWORKER_END_STATUS_H
#error "Clean your dependencies"
#endif
#define DXPB_BWORKER_END_STATUS_H

enum end_status {
	END_STATUS_OK,
	END_STATUS_OBSOLETE,
	END_STATUS_PKGFAIL,
	END_STATUS_NODISK,
	END_STATUS_NOMEM,
	END_STATUS_WRONG_ASSIGNMENT,
	END_STATUS_NUM_MAX
};
