/*
 * This is a horrible hack.
 * An executable shell script that can be setuid root so that root privs are not required for mounting filesystem images.
 * Since this is intended to be a setuid root binary, be very careful what you do here.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <fcntl.h>

#define MOUNT_PATH "/bin/mount"

extern char **environ;

int main(int argc, char *argv[])
{
	char *image_file = NULL, *mount_point = NULL;
	char *nenviron[] = { NULL };
	char *nargv[] = { MOUNT_PATH, "-o", "loop", NULL, NULL};

	if(argc != 3)
	{
		fprintf(stderr, "\nUsage: %s <image> <mount point>\n\n", argv[0]);
		return EXIT_FAILURE;
	}
	else
	{
		image_file = argv[1];
		mount_point = argv[2];
		nargv[3] = image_file;
		nargv[4] = mount_point;
	}
	
	if(mkdir(mount_point, 0) != 0)
	{
		perror(mount_point);
	}
	
	setuid(0);
	execve(nargv[0], nargv, nenviron);

	return EXIT_FAILURE;
}

