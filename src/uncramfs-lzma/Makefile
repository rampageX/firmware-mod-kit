PROG = uncramfs-lzma
CC = gcc -O3 -w
LIB = -lm
RM = rm -f
CFLAGS = -c

OBJS = \
  lzma-uncramfs.o \
  lzma-rg/SRC/7zip/Compress/LZMA_C/decode.o \
  lzma-rg/SRC/7zip/Compress/LZMA_C/LzmaDecode.o \


all: $(PROG)

$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(LDFLAGS) $(OBJS) $(LIB)

lzma-uncramfs.o: lzma-uncramfs.c
	$(CC) $(CFLAGS) lzma-uncramfs.c

lzma-rg/SRC/7zip/Compress/LZMA_C/decode.o:
	cd lzma-rg/SRC/7zip/Compress/LZMA_C;make

clean:
	-$(RM) $(PROG) $(OBJS)
	cd lzma-rg/SRC/7zip/Compress/LZMA_C;make clean

