CC=gcc

CFLAGS := -I. -Ibrcm-lzma -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_GNU_SOURCE -O2

all: unsquashfs

unsquashfs: nb4-unsquash.o
	cd brcm-lzma; make
	gcc nb4-unsquash.o brcm-lzma/7zlzma.o brcm-lzma/IInOutStreams.o brcm-lzma/LZMADecoder.o -lpthread -lm -o $@

nb4-unsquash.o: nb4-unsquash.c squashfs_fs.h read_fs.h global.h

clean:
	cd brcm-lzma; make clean
	rm -f nb4-unsquash.o unsquashfs
