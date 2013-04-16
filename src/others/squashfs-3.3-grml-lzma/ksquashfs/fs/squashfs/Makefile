#
# Makefile for the linux squashfs routines.
#

# the environment variables are not inherited since 2.6.23
ifdef SQLZMA_EXTRA_CFLAGS
EXTRA_CFLAGS += ${SQLZMA_EXTRA_CFLAGS}
endif

obj-$(CONFIG_SQUASHFS) += squashfs.o
squashfs-y += inode.o
squashfs-y += squashfs2_0.o
