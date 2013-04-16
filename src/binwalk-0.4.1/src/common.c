#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include "common.h"

#ifdef __linux
#include <linux/fs.h>
#endif

/* Convert decimal and hexadecimal strings to integers */
int str2int(char *str)
{
        int val = 0;
        int strsize = 0;
        int base = 10;
        char *strptr = NULL;
        char buffer[INT_MAX_STR_SIZE] = { 0 };

	if(str != NULL)
	{
        	strsize = strlen(str);

        	if(strsize < INT_MAX_STR_SIZE)
        	{

        		memcpy((void *) &buffer, str, strsize);
        		strptr = (char *) &buffer;
	
        		/* If the string starts with '0x' or '\x', or if it ends in 'h' or 'H', treat it as hex */
        		if(strsize >= 2)
        		{
        		        if(strptr[1] == 'x')
        		        {
        		                strptr += 2;
        		                base = 16;
        		        }
        		        else if(strptr[strsize-1] == 'h' || strptr[strsize-1] == 'H')
        		        {
        		                strptr[strsize-1] = '\0';
        		                base = 16;
        		        }
        		}
	
	        	val = strtol(strptr,NULL,base);
		}
	}

        return val;
}

/* Reads in and returns the contents and size of a given file */
const void *file_read(char *file, size_t *fsize)
{
        int fd = 0;
	size_t file_size = 0;
        struct stat _fstat = { 0 };
        const void *buffer = NULL;

	fd = open(file, O_RDONLY);
        if(!fd)
        {
                perror(file);
                goto end;
        }

        if(stat(file, &_fstat) == -1)
        {
		perror(file);
                goto end;
        }

        if(_fstat.st_size > 0)
        {
		file_size = _fstat.st_size;
        }
#ifdef __linux
	else
	{
		long long long_file_size = 0;

		/* Special files may report a zero size in stat(); must get their file size via an ioctl call */
		if(ioctl(fd, BLKGETSIZE64, &long_file_size) == -1)
		{
			perror("ioctl");
			goto end;
		}
		else
		{
			file_size = (size_t) long_file_size;
		}
	}
#endif

	if(file_size > 0)
	{
		buffer = mmap(NULL, file_size, PROT_READ, (MAP_SHARED | MAP_NORESERVE), fd, 0);
		if(buffer == MAP_FAILED)
		{
			perror("mmap");
			buffer = NULL;
		}
		else
		{
			*fsize = file_size;
		}
	}

end:
        if(fd) close(fd);
        return buffer;
}

/* Print messages to both the log file and stdout, as appropriate */
void print(const char* format, ...)
{
        va_list args;

        va_start(args,format);

        if(globals.fsout != NULL)
        {
                vfprintf(globals.fsout,format,args);
                fflush(globals.fsout);
        }
        if(globals.quiet == 0)
        {
                vfprintf(stdout,format,args);
                fflush(stdout);
        }

        va_end(args);
        return;
}

/* Returns the current timestamp as a string */
char *timestamp()
{
        time_t t = { 0 };
        struct tm *tmp = NULL;
        char *ts = NULL;

        t = time(NULL);
        tmp = localtime(&t);
        if(!tmp)
        {
                perror("Localtime failure");
                goto end;
        }

        ts = malloc(MAX_TIMESTAMP_SIZE);
        if(!ts)
        {
                perror("Malloc failure");
                goto end;
        }
        memset(ts,0,MAX_TIMESTAMP_SIZE);

        if(strftime(ts,MAX_TIMESTAMP_SIZE-1,TIMESTAMP_FORMAT,tmp) == 0)
        {
                if(ts) free(ts);
                ts = NULL;
        }

end:
        return ts;
}

