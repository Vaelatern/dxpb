/*
 * brepowatch.h
 *
 * Agent and helper for keeping track of the binary package repository.
 * Does not have handling for a directory being deleted.
 */

pthread_t	*brepowatch_filefinder_prepare(zsock_t **, const char *, const char *);
