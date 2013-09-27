/*
 * This is a horrible hack.
 * An executable shell script that can be setuid root so that root privs are not required for unmounting filesystem images.
 * Since this is intended to be a setuid root binary, be very careful what you do here.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#define UMOUNT_PATH "/bin/umount"

int main(int argc, char *argv[])
{
	char *nenviron[] = { NULL };
	char *nargv[] = { UMOUNT_PATH, NULL};

	if(argc != 2)
	{
		fprintf(stderr, "\nUsage: %s <mount point>\n\n", argv[0]);
		return EXIT_FAILURE;
	}
	else
	{
		nargv[1] = argv[1];
	}
	
	setuid(0);
	execve(nargv[0], nargv, nenviron);

	return EXIT_FAILURE;
}

