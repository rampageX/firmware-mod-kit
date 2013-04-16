# Modifications were made by Cisco Systems, Inc. on or before Thu Feb 16 11:28:44 PST 2012
CC = gcc

LZMADIR = ../lzma/C

CFLAGS := -I $(LZMADIR)  -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_GNU_SOURCE -O2

LZMAOBJS = LzmaEnc.o LzFind.o LzmaDec.o
MKOBJS = mksquashfs.o read_fs.o sort.o lzmainterface.o

all: mksquashfs unsquashfs

$(LZMAOBJS): %.o: $(LZMADIR)/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $^ -o $@

mksquashfs: $(MKOBJS) $(LZMAOBJS)
	$(CC) $(MKOBJS) $(LZMAOBJS) -lz -lpthread -lm -lstdc++ -o $@

mksquashfs.o: mksquashfs.c squashfs_fs.h mksquashfs.h global.h sort.h

read_fs.o: read_fs.c squashfs_fs.h read_fs.h global.h

sort.o: sort.c squashfs_fs.h global.h sort.h
unsquashfs: unsquashfs.o $(LZMAOBJS)
	$(CC) unsquashfs.o  lzmainterface.o $(LZMAOBJS) -lz -lpthread -lm -o $@

unsquashfs.o: unsquashfs.c squashfs_fs.h read_fs.h global.h
clean:
	-rm -f *.o mksquashfs unsquashfs

install: mksquashfs unsquashfs
	mkdir -p $(INSTALL_DIR)
	cp mksquashfs unsquashfs $(INSTALL_DIR)
