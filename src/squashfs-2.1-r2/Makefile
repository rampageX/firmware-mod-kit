CC := gcc
CXX := g++
INCLUDEDIR = .
CFLAGS := -I$(INCLUDEDIR) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -O2
LZMAPATH = ../lzma/C/7zip/Compress/LZMA_Lib

all: mksquashfs-lzma mksquashfs unsquashfs-lzma unsquashfs

mksquashfs: mksquashfs.o read_fs.o sort.o
	$(CC) mksquashfs.o read_fs.o sort.o -lz -o $@

mksquashfs-lzma: mksquashfs.o read_fs.o sort.o
	make -C $(LZMAPATH)
	$(CXX) -O3 mksquashfs.o read_fs.o sort.o -L$(LZMAPATH) -llzma -o $@

mksquashfs.o: mksquashfs.c squashfs_fs.h mksquashfs.h

read_fs.o: read_fs.c squashfs_fs.h read_fs.h 

sort.o: sort.c squashfs_fs.h 

unsquashfs-lzma: unsquashfs.o
	make -C $(LZMAPATH)
	$(CXX) -O3 unsquashfs.o -L$(LZMAPATH) -llzma -o $@

unsquashfs: unsquashfs.o
	$(CC) unsquashfs.o -lz -o $@

unsquashfs.o: unsquashfs.c squashfs_fs.h read_fs.h

clean:
	rm -f *.o
	rm -f mksquashfs
	rm -f unsquashfs
	rm -f mksquashfs-lzma
	rm -f unsquashfs-lzma
	rm -f mksquashfs.exe
	rm -f unsquashfs.exe
	rm -f mksquashfs-lzma.exe
	rm -f unsquashfs-lzma.exe
	make -C $(LZMAPATH) clean
