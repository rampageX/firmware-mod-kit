
CC = gcc
CXX = g++
AR = ar
CFLAGS = -W -Wall -O2 -g

INCLUDEDIR = .

CFLAGS := -I$(INCLUDEDIR) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -g

ifdef CONFIG_SQUASHFS_CRAMFS_MAGIC
CFLAGS += -DCONFIG_SQUASHFS_CRAMFS_MAGIC=1
endif

all: mksquashfs unsquashfs

mksquashfs: mksquashfs.o read_fs.o sort.o liblzma.a
	$(CXX) mksquashfs.o read_fs.o sort.o -L. -llzma -lpthread -o $@

mksquashfs.o: mksquashfs.c mksquashfs.h

liblzma.a: liblzma/Makefile
	$(MAKE) -f liblzma/Makefile

unsquashfs: unsquashfs.o liblzma.a
	$(CXX) unsquashfs.o -L. -llzma -lpthread -o $@

#unsquashfs.o: unsquashfs.c

read_fs.o: read_fs.c read_fs.h

sort.o: sort.c

clean:
	rm -f *.o liblzma.a unsquashfs mksquashfs
