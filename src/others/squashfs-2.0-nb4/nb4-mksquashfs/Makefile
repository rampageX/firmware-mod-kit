LZMADIR = ./lzma/compress
INCLUDEDIR = -I. -I$(LZMADIR)
LZMAOBJS = \
	7zapi.o \
        7zlzma.o \
        AriBitCoder.o \
        IInOutStreams.o \
        LenCoder.o \
        LiteralCoder.o \
        LZMA.o \
        LZMAEncoder.o \
        OutByte.o \
        WindowIn.o

CC = gcc
STRIP = strip

#CFLAGS		= $(DEFS) $(WARNINGS) -O2 -D__MACOSX__ -DGNU $(INCLUDEDIR)
CFLAGS		= $(DEFS) $(WARNINGS) -O2 -DGNU $(INCLUDEDIR)

vpath %.cpp $(LZMADIR)

mksquashfs:  squashfs/*.c squashfs/*.h $(LZMAOBJS) 
	@if [ ! -e /usr/include/zlib.h ]; then \
          echo *****You need to install zlib-devel package to build mksquashfs! ; \
          echo *****You may find it in your Linux distribution CDs. ; \
	  exit ;\
        fi
	gcc $(CFLAGS) -Isquashfs -c squashfs/*.c
	gcc $(CFLAGS) -c getline.c
	gcc $(CFLAGS) -c getdelim.c
	g++  -o mksquashfs mksquashfs.o read_fs.o sort.o getline.o getdelim.o $(LZMAOBJS) -lz
#	ln mksquashfs nb4-mksquashfs

$(LZMAOBJS): %.o: %.cpp
	g++ -c $(CFLAGS) $^ -o $@

clean:
	rm -f mksquashfs nb4-mksquashfs
	rm -fr *.o core
	rm -fr $(LZMADIR)/*.o 
