INCLUDEDIR = -I. -I./lzma/C 
LZMAPATH = ./lzma/C/LzmaLib

USE_LZMA = 1

ifdef USE_LZMA
  LZMA_CFLAGS = -DUSE_LZMA
  LZMA_LIB = -L$(LZMAPATH) -llzma
  CFLAGS += $(LZMA_CFLAGS)
endif

CFLAGS := $(INCLUDEDIR) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_GNU_SOURCE -O2

USE_LZMA = 1

ifdef USE_LZMA
  LZMA_CFLAGS = -DUSE_LZMA
  LZMA_LIB = -L$(LZMAPATH) -llzma
  CFLAGS += $(LZMA_CFLAGS)
endif

all: mksquashfs-lzma unsquashfs-lzma

mksquashfs-lzma: mksquashfs.o read_fs.o sort.o swap.o pseudo.o uncompress.o
	make -C $(LZMAPATH)
	$(CC) mksquashfs.o read_fs.o sort.o swap.o pseudo.o uncompress.o -lz -lpthread -lm $(LZMA_LIB) -o $@

mksquashfs.o: mksquashfs.c squashfs_fs.h mksquashfs.h global.h sort.h squashfs_swap.h uncompress.h Makefile

read_fs.o: read_fs.c squashfs_fs.h read_fs.h global.h squashfs_swap.h uncompress.h Makefile

sort.o: sort.c squashfs_fs.h global.h sort.h Makefile

swap.o: swap.c Makefile

pseudo.o: pseudo.c pseudo.h Makefile

uncompress.o: uncompress.c uncompress.h

unsquashfs-lzma: unsquashfs.o unsquash-1.o unsquash-2.o unsquash-3.o unsquash-4.o swap.o uncompress.o
	$(CC) unsquashfs.o unsquash-1.o unsquash-2.o unsquash-3.o unsquash-4.o swap.o uncompress.o -lz -lpthread -lm $(LZMA_LIB) -o $@

unsquashfs.o: unsquashfs.h unsquashfs.c squashfs_fs.h squashfs_swap.h squashfs_compat.h global.h uncompress.h Makefile

unsquash-1.o: unsquashfs.h unsquash-1.c squashfs_fs.h squashfs_compat.h global.h uncompress.h Makefile

unsquash-2.o: unsquashfs.h unsquash-2.c unsquashfs.h squashfs_fs.h squashfs_compat.h global.h uncompress.h Makefile

unsquash-3.o: unsquashfs.h unsquash-3.c squashfs_fs.h squashfs_compat.h global.h uncompress.h Makefile

unsquash-4.o: unsquashfs.h unsquash-4.c squashfs_fs.h squashfs_swap.h global.h uncompress.h Makefile

clean:
	-rm -f *.o mksquashfs-lzma unsquashfs-lzma

install: mksquashfs unsquashfs
	mkdir -p $(INSTALL_DIR)
	cp mksquashfs $(INSTALL_DIR)
	cp unsquashfs $(INSTALL_DIR)
