/*
 * Utility for calculating and patching checksums in various files.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include "crcalc.h"
#include "patch.h"

int main(int argc, char *argv[])
{
	int retval = EXIT_FAILURE, ok = 0, fail = 1, n = 0, i = 0, offset = 0;
	int offsets[MAX_HEAD_SIZE] = { 0 };
	char *buf = NULL, *ptr = NULL, *fname = NULL, *log = NULL;
	size_t size = 0, nsize = 0;

	/* Check usage */
	if(argc < 2 || argv[1][0] == '-')
	{
		fprintf(stderr, USAGE, argv[0]);
		goto end;
	}
	else
	{
		fname = argv[1];
	
		if(argc == 3)
		{
			log = argv[2];
		}
	}

	/* Read in target file */
	buf = file_read(fname, &size);

	if(buf && size > MIN_FILE_SIZE)
	{
		/* Parse in the log file, if any */
		n = parse_log(log, offsets);

		fprintf(stderr, "Processing %d header(s) from %s...\n", n, fname);

		/* Loop through each offset in the integer array */
		for(i=0; i<n; i++)
		{
			ok = 0;
			offset = offsets[i];
			nsize = size - offset;
			ptr = (buf + offset);

			fprintf(stderr, "Processing header at offset %d...", offset);

			/* Identify and patch the header at each offset */
			switch(identify_header(ptr))
			{
				case TRX:
					ok = patch_trx(ptr, nsize);
					break;
				case UIMAGE:
					ok = patch_uimage(ptr, nsize);
					break;
				case DLOB:
					ok = patch_dlob(ptr, nsize);
					break;
				default:
					fprintf(stderr, "sorry, this file type is not supported.\n");
					break;
			}

			if(ok)
			{
				fail = 0;
				fprintf(stderr, "checksum(s) updated OK.\n");
			}
			else
			{
				fprintf(stderr, "checksum update(s) failed!\n");
			}
		}
	}

	if(!fail)
	{
		if(!file_write(fname, buf, size))
		{
			fprintf(stderr, "Failed to save data to file '%s'\n", fname);
		}
		else
		{
			fprintf(stderr, "CRC(s) updated successfully.\n");
			retval = EXIT_SUCCESS;
		}
	}
	else
	{
		fprintf(stderr, "CRC update failed.\n");
	}

end:
	if(buf) free(buf);
	return retval;
}

/* Parse binwalk-style log file for offsets to headers in the target firmware image */
int parse_log(char *file, int offsets[MAX_HEAD_SIZE])
{
	FILE *fp = NULL;
	char line[MAX_LINE_SIZE] = { 0 };
	int n = 0;

	if(file == NULL)
	{
		offsets[0] = 0;
		n = 1;
	}
	else
	{
		fp = fopen(file, "r");
		if(fp)
		{
			/* Read each line one at a time from the log file */
			while((fgets((char *) &line, MAX_LINE_SIZE, fp) != NULL) && (n < MAX_HEAD_SIZE))
			{
				/* 
				 * If the line has the string 'header', or it is a short, non-white space line, 
				 * use the offset in this line. This allows us to parse a file with just a list of
				 * header offsets, as well as standard binwalk logs.
				 */
				if((((strlen((char *) &line) < MAX_STR_SIZE) && !is_whitespace((char *) &line))) ||
				   (strstr((char *) &line, HEADER_ID_STR)))
				{
					offsets[n] = atoi((char *) &line);
					n++;
				}

				memset((char *) &line, 0, MAX_LINE_SIZE);
			}

			fclose(fp);
		}
		else
		{
			perror(file);
		}
	}

	return n;
}

/* Determine if a string is all white space or not */
int is_whitespace(char *string)
{
	int i = 0, retval = 1;

	for(i=0; i<strlen(string); i++)
	{
		if(string[i] >= MIN_NON_WHITE)
		{
			retval = 0;
			break;
		}
	}

	return retval;
}

/* Reads in and returns the contents and size of a given file */
char *file_read(char *file, size_t *fsize)
{
        int fd = 0;
        struct stat _fstat = { 0 };
        char *buffer = NULL;

        if(stat(file, &_fstat) == -1)
        {
                perror(file);
                goto end;
        }

        if(_fstat.st_size == 0)
        {
                fprintf(stderr, "%s: zero size file\n", file);
                goto end;
        }

        fd = open(file,O_RDONLY);
        if(!fd)
        {
                perror(file);
                goto end;
        }

        buffer = malloc(_fstat.st_size);
        if(!buffer)
        {
                perror("malloc");
		goto end;
        }
        memset(buffer, 0 ,_fstat.st_size);

        if(read(fd, buffer, _fstat.st_size) != _fstat.st_size)
        {
                perror(file);
                if(buffer) free(buffer);
                buffer = NULL;
        }
        else
        {
                *fsize = _fstat.st_size;
        }

end:
        if(fd) close(fd);
        return buffer;
}

/* Write size bytes from buf to file fname */
int file_write(char *fname, char *buf, size_t size)
{
	FILE *fp = NULL;
	int retval = 0;

	fp = fopen(fname, "w");
	if(fp)
	{
		if(fwrite(buf, 1, size, fp) == size)
		{
			retval = 1;
		}
		
		fclose(fp);
	}
	else
	{
		perror("fopen");
	}

	return retval;
}

/* Identifies the header type used in the supplied file */
enum header_type identify_header(char *buf)
{
	enum header_type retval = UNKNOWN;
	uint32_t *sig = NULL;

	sig = (uint32_t *) buf;

	switch(*sig)
	{
		case TRX_MAGIC:
			retval = TRX;
			break;
		case UIMAGE_MAGIC:
			retval = UIMAGE;
			break;
		case DLOB_MAGIC:
			retval = DLOB;
			break;
		default:
			break;
	}

	return retval;
}

