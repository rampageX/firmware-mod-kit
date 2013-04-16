CC = gcc

SRCBASE = .
INCLUDEDIR = -I$(SRCBASE)/include -I.
LZMADIR = $(SRCBASE)/lzma_src/C

CFLAGS := $(INCLUDEDIR) -I$(LZMADIR) -I$(LINUXDIR)/include -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -D_GNU_SOURCE -O2

LZMAOBJS = LzmaEnc.o LzFind.o LzmaDec.o
MKOBJS = mksquashfs.o read_fs.o sort.o sqlzma.o

all: mksquashfs unsquashfs

$(LZMAOBJS): %.o: $(LZMADIR)/%.c
	$(CC) -c $(CFLAGS) $(CPPFLAGS) $^ -o $@

mksquashfs: $(MKOBJS) $(LZMAOBJS)
	$(CC) $(MKOBJS) $(LZMAOBJS) -lz -lpthread -lm -lstdc++ -o $@

unsquashfs: unsquashfs.o $(LZMAOBJS)
	$(CC) unsquashfs.o  sqlzma.o $(LZMAOBJS) -lz -lpthread -lm -o $@

clean:
	-rm -f *.o mksquashfs unsquashfs

install: mksquashfs unsquashfs
	mkdir -p $(INSTALL_DIR)
	cp mksquashfs unsquashfs $(INSTALL_DIR)
