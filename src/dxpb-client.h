typedef int (*setssl_cb)(void *, const char *, const char *, const char *);
int		setup_ssl(void *, setssl_cb, const char *, const char *, const char *);
