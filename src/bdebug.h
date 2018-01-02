#define FULLDEBUG 1

#ifdef FULLDEBUG
#define printnote(x) fprintf(stderr, "%s\n", x)
#else
#define printnote(x)
#endif
