/*
 * This is a horrible hack.
 * An executable shell script that can be setuid root so that root privs are not required for mounting jffs2 images.
 * Since this is intended to be a setuid root binary, be very careful what you do here. Don't take in user-supplied content.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/sysmacros.h>
#include <unistd.h>
#include <fcntl.h>

int openfile(void)
{
	int fd = 0, retval = 0;

	/* Validate that the jffs2.img file exists. */
	fd = open("jffs2.img", 0);
	if(fd < 0)
	{
		perror("jffs2.img");
		retval = -1;
	}
	else
	{
		close(fd);
	}

	return retval;
}

int mkdirs(void)
{
	int retval = 0;

	/* Create the mount and output directories. */
	if(mkdir("jffs2-root", 0777) != 0)
	{
		perror("jffs2-root");
		retval = -1;
	}

	if(mkdir("rootfs", 0777) != 0)
	{
		perror("rootfs");
		retval = -1;
	}

	return retval;
}

void modprobe(void)
{
	system("modprobe mtdblock");
	system("modprobe mtdchar");
	system("modprobe mtd");
	system("modprobe mtdram");
	system("rmmod mtdram");
	system("modprobe mtdram total_size=65536");
}

void mount(void)
{
	system("mknod /dev/mtdblock1 b 31 0");
	system("dd if=jffs2.img of=/dev/mtdblock1");
	system("mount -t jffs2 /dev/mtdblock1 jffs2-root");
}

void copy(void)
{
	system("cp -R jffs2-root/* rootfs/");
}

void umount(void)
{
	system("umount jffs2-root");
	system("rm -rf jffs2-root");
}

int main(int argc, char *argv[])
{
	if(argc > 1)
	{
		fprintf(stderr, "\nLooks for a file name 'jffs2.img' and extracts its contents to a directory named 'rootfs'.\n");
		fprintf(stderr, "Usage: %s\n\n", argv[0]);
		return EXIT_FAILURE;
	}
	
	if(openfile() != 0 || mkdirs() != 0)
	{
		return EXIT_FAILURE;
	}
	else
	{
		setuid(0);
		modprobe();
		mount();
		copy();
		umount();
	}

	return EXIT_SUCCESS;
}
