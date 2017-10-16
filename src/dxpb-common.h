// Remove the following to cause errors when ssldir is ready to be rolled out.
#define SSLDIR_UNUSED(x) (void)x

void	prologue(const char *);
void	print_license(void);
void	flushsock(void *, const char *);

#define DEFAULT_SSLDIR "/etc/dxpb/curve/"

#define DEFAULT_IMPORT_ENDPOINT "ipc:///var/run/dxpb/pkgimport.sock"
#define DEFAULT_FILE_ENDPOINT   "tcp://127.0.0.1:5196"
#define DEFAULT_GRAPH_ENDPOINT  "tcp://127.0.0.1:5195"

#define DEFAULT_DBPATH "/var/cache/dxpb/pkgs.db"

#define DEFAULT_STAGINGDIR "/var/lib/dxpb/staging/"
#define DEFAULT_LOGDIR     "/var/lib/dxpb/logs/"
#define DEFAULT_REPODIR    "/var/lib/dxpb/pkgs/"

#define DEFAULT_REPOPATH "./"

#define DEFAULT_XBPS_SRC "./xbps-src"
#define	DEFAULT_MASTERDIR "./masterdir"
#define	DEFAULT_HOSTDIR   "./hostdir"
