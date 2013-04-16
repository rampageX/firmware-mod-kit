#ifndef _CRCALC_H_
#define _CRCALC_H_

#include <stdint.h>

#define MIN_FILE_SIZE 4
#define MAX_LINE_SIZE 8096
#define MAX_HEAD_SIZE 512
#define HEADER_ID_STR "header"
#define MIN_NON_WHITE '!'
#define MAX_STR_SIZE  10
#define MIN_STR_SIZE  1

#define USAGE "\n\
crcalc v0.2 - (c) 2011, Craig Heffner\n\
Re-calculates firmware header checksusms for TRX and uImage firmware headers.\n\
\n\
Usage: %s <firmware image> [binwalk log file]\n\
\n\
If no binwalk log file is specified, the header is assumed to be at the beginning of the firmware image.\n\
\n"

enum header_type
{
	UNKNOWN,
	TRX,
	UIMAGE,
	DLOB,
};

int parse_log(char *file, int offsets[MAX_HEAD_SIZE]);
int is_whitespace(char *string);
char *file_read(char *file, size_t *fsize);
int file_write(char *fname, char *buf, size_t size);
enum header_type identify_header(char *buf);

#endif
