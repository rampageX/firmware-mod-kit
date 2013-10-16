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
#include "common.h"
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

