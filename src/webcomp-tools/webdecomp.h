#ifndef _WEBDECOMP_H_
#define _WEBDECOMP_H_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#define NONE 	0
#define EXTRACT 1
#define RESTORE 2

#define USAGE "\
webdecomp v.0.5, (c) 2011, Craig Heffner\n\
\n\
Extracts and restores the Web UI pages from and to DD-WRT firmware.\n\
\n\
Usage: %s [OPTIONS]\n\
\n\
Required Options:\n\
\n\
\t-b, --httpd=<file>                    Path to the DD-WRT httpd binary (ex: usr/sbin/httpd)\n\
\t-w, --www=<file>                      Path to the DD-WRT www binary (ex: etc/www)\n\
\t-k, --key=<int>                       The key value to use when extracting/restoring files\n\
\t-e, --extract                         Extract Web files to a directory\n\
\t-r, --restore                         Restore Web files from a directory\n\
\n\
Additional Options:\n\
\n\
\t-i, --index=<offset>                  File offset of the websRomPageIndex structure array\n\
\t-d, --dir=<directory>                 Web files directory [default: %s]\n\
\t-h, --help                            Show help\n\
"

void usage(char *progname);
int restore(char *httpd, char *www, char *dir);
int extract(char *httpd, char *www, char *outdir);
int detect_settings(char *httpd_file);
void detect_key(char *httpd, char *www);

#endif
