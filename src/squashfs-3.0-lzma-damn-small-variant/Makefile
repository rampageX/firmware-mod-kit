INCLUDEDIR = .
LZMAPATH = ./lzma/C/7zip/Compress/LZMA_Lib

CFLAGS := -I$(INCLUDEDIR) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -funroll-loops -O3 -D_LZMA_PARAMS -w

all: mksquashfs-lzma unsquashfs-lzma

mksquashfs: mksquashfs.o read_fs.o sort.o
	$(CC) mksquashfs.o read_fs.o sort.o -lz -o $@

mksquashfs-lzma: mksquashfs.o read_fs.o sort.o
	make -C $(LZMAPATH)
	$(CXX) mksquashfs.o read_fs.o sort.o -L$(LZMAPATH) -llzma -lpthread -o $@

mksquashfs.o: mksquashfs.c squashfs_fs.h mksquashfs.h global.h sort.h

read_fs.o: read_fs.c squashfs_fs.h read_fs.h global.h

sort.o: sort.c squashfs_fs.h global.h sort.h

unsquashfs: unsquashfs.o
	$(CC) unsquashfs.o -lz -o $@

unsquashfs-lzma: unsquashfs.o
	$(CXX) $(CFLAGS) unsquashfs.o -L$(LZMAPATH) -llzma -lpthread -o $@ 

unsquashfs.o: unsquashfs.c squashfs_fs.h read_fs.h global.h

clean:
	make -C $(LZMAPATH) clean
	rm -f *.o 
	rm -f mksquashfs-lzma
	rm -f unsquashfs-lzma
