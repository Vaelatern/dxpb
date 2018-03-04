#define TOMSG(type, what) PKG##type##_MSG_##what

#define SEND(type, TYPE, WHAT, msg, target) { \
		pkg##type##_msg_set_id(msg, TOMSG(TYPE, WHAT)); \
		rc = pkg##type##_msg_send(msg, target); \
		assert(rc == 0 && "message sending didn't work"); \
	}
#define ISEND(WHAT) SEND(import, IMPORT, WHAT, import_msg, import)
#define GSEND(WHAT) SEND(graph, GRAPH, WHAT, graph_msg, graph)
#define FSEND(WHAT) SEND(files, FILES, WHAT, file_msg, file)

#define SET(type, what, msg, to) pkg##type##_msg_set_##what(msg, to)
#define ISET(what, to) SET(import, what, import_msg, to)
#define GSET(what, to) SET(graph, what, graph_msg, to)
#define FSET(what, to) SET(files, what, file_msg, to)

#define GET(type, mymsg, sock) { \
		zpoller_t *p = zpoller_new(sock, NULL); \
		(void) zpoller_wait(p, 30*1000); \
		assert(!zpoller_expired(p) && "Timeout while waiting for any message"); \
		if (zpoller_terminated(p)) \
			exit(-1); \
		rc = pkg##type##_msg_recv(mymsg, sock); \
		assert(rc == 0 && "We thought there was a message. We were wrong"); \
		zpoller_destroy(&p); \
	}
#define IGET()	GET(import, import_msg, import)
#define GGET()	GET(graph, graph_msg, graph)
#define FGET()	GET(files, file_msg, file)

#define IF(type, msg, field, goal) if(pkg##type##_msg_##field(msg) == goal)
#define IIF(field, goal) IF(import, import_msg, field, goal)
#define GIF(field, goal) IF(graph, graph_msg, field, goal)
#define FIF(field, goal) IF(files, file_msg, field, goal)
#define IIFID(goal) IIF(id, TOMSG(IMPORT, goal))
#define GIFID(goal) GIF(id, TOMSG(GRAPH, goal))
#define FIFID(goal) FIF(id, TOMSG(FILES, goal))

// SRT is shorthand for assert(), and STRS, for assert(strcmp())
#define SRT(type, msg, field, goal) assert(pkg##type##_msg_##field(msg) == goal)
#define ISRT(field, goal) SRT(import, import_msg, field, goal)
#define GSRT(field, goal) SRT(graph, graph_msg, field, goal)
#define FSRT(field, goal) SRT(files, file_msg, field, goal)
#define ISRTID(goal) SRT(import, import_msg, id, TOMSG(IMPORT, goal))
#define GSRTID(goal) SRT(graph, graph_msg, id, TOMSG(GRAPH, goal))
#define FSRTID(goal) SRT(files, file_msg, id, TOMSG(FILES, goal))

#define SRTS(type, msg, field, goal) assert(strcmp(pkg##type##_msg_##field(msg), goal) == 0)
#define ISRTS(field, goal) SRTS(import, import_msg,  field , goal)
#define GSRTS(field, goal) SRTS(graph, graph_msg,  field , goal)
#define FSRTS(field, goal) SRTS(files, file_msg,  field , goal)
