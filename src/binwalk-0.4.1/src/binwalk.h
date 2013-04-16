#ifndef BINWALK_H
#define BINWALK_H

#include <magic.h>
#include "md5.h"
#include "mparse.h"
#include "filter.h"
#include "common.h"
#include "config.h"

/* These should get defined by the Makefile. If not, default to /etc/binwalk/magic.bin*. */
#ifndef MAGIC
#define MAGIC			"/etc/binwalk/magic.binwalk"
#endif
#ifndef MAGIC_CAST
#define MAGIC_CAST		"/etc/binwalk/magic.bincast"
#endif
#ifndef MAGIC_ARCH
#define MAGIC_ARCH		"/etc/binwalk/magic.binarch"
#endif

#define DATA			"data"
#define DATA_SIZE		4
#define TEXT			"text"
#define DEFAULT_BYTE_ALIGN	1
#define PROGRESS_INTERVAL	1000
#define MAX_SIGNATURES		8192
#define MULTIPLE_MATCH_DELIM	"\\012- "
#define MULTIPLE_MATCH_NEWLINE	"\r\n\t\t\t\t"
#define MULTIPLE_MATCH_SIZE	6
#define GZIP_FILTER		"gzip"
#define LZMA_FILTER		"lzma"
#define JFFS_FILTER		"jffs2"
#define INVALID_FILTER		"invalid"

#define USAGE_OPTIONS		"\
OPTIONS\n\
\t-o, --offset=<int>            File offset to start searching at\n\
\t-l, --length=<int>            Number of bytes to search\n\
\t-b, --align=<int>             Set byte alignment\n\
\t-f, --file=<file>             Log results to file\n\
\t-m, --magic=<file>            Magic file to use [%s]\n\
\t-y, --search=<filter>         Only search for matches that have <filter> in their description (implies -n, -t)\n\
\t-x, --exclude=<filter>        Exclude matches that have <filter> in their description\n\
\t-i, --include=<filter>        Include matches that are normally excluded and that have <filter> in their description *\n\
\t-a, --all                     Search for all matches, including those that are normally excluded *\n\
\t-d, --defaults                Speed up scan by disabling default filters **\n\
\t-t, --fast                    Speed up scan by only loading signatures specified by -i or -y\n\
\t-u, --update                  Update magic signature files\n\
\t-v, --verbose                 Enable verbose mode\n\
\t-s, --smart                   Disable smart matching (implies -a)\n\
\t-k, --keep-going              Don't stop at the first match\n\
\t-c, --validate                Validate magic file\n\
\t-q, --quiet                   Supress output to stdout\n\
\t-A, --opcodes                 Scan for executable code (implies -a)\n\
\t-C, --cast                    Cast file contents as various data types (implies -k)\n\
\n\n\
*  Signatures of two bytes or less are excluded by default. Use -i or -a to include them in the search.\n\
\n\
** Default filters include '%s', '%s' and '%s' results, and exclude '%s' results. Disabling the default\n\
   filters will speed up scan time, but may miss these file types.\n\
"

struct magic_signature
{
	int offset;
	int size;
	int wildcard;
	char *signature;
};

struct binconf
{
	int smart;
	int verbose;
	int flags;
	int offset;
	int align;
	int length;
	char *magic;
	magic_t cookie;
};

void usage(char *progname);
int process_file(char *bin_file, struct binconf *config, struct magic_signature **signatures, int num_sigs, struct magic_filter **filters, int filter_count);

#endif
