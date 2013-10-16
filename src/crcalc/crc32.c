#include <stdio.h>
#include <stdlib.h>
#include "crc.h"

int main(int argc, char *argv[])
{
	FILE *fp = NULL;
	char *buf = NULL, *target = NULL;
	int retval = EXIT_FAILURE, offset = 0, len = 0, crc = 0;

	if(argc > 1)
	{
		switch(argc)
		{
			case 4:
				len = atoi(argv[3]);
			case 3:
				offset = atoi(argv[2]);
			case 2:
				target = argv[1];
		}
	}
	else
	{
		fprintf(stderr, "Usage: %s <file> [offset] [length]\n", argv[0]);
		goto end;
	}

	fp = fopen(target, "rb");
	if(fp)
	{
		fseek(fp, offset, SEEK_SET);

		if(len == 0)
		{
			fseek(fp, 0L, SEEK_END);
			len = ftell(fp) - offset;
			fseek(fp, offset, SEEK_SET);
		}

		buf = malloc(len);
		if(!buf)
		{
			perror("malloc");
			goto end;
		}

		if(fread(buf, 1, len, fp) == len)
		{
			crc = crc32(buf, len);
			printf(" CRC32: 0x%X\n", crc);
			printf("~CRC32: 0x%X\n", (unsigned int) (crc ^ 0xFFFFFFFFL));
		
			retval = EXIT_SUCCESS;
		}
		else
		{
			perror("fread");
		}
	}
	else
	{
		perror(target);
	}

end:
	if(buf) free(buf);
	if(fp) fclose(fp);
	return retval;
}
