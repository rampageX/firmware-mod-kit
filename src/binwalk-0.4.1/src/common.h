#ifndef COMMON_H
#define COMMON_H

#define INT_MAX_STR_SIZE        1024
#define MAX_TIMESTAMP_SIZE      1024
#define TIMESTAMP_FORMAT        "%b %d, %Y @ %T"

/* These globals are set once, read many */
struct bin_globals
{
        FILE *fsout;
        int quiet;
} globals;

int str2int(char *str);
const void *file_read(char *file, size_t *fsize);
void print(const char* format, ...);
char *timestamp();

#endif
