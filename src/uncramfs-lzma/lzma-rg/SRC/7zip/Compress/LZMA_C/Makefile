PROG = decode
CC = gcc -O3 -Wall
LIB = -lm
RM = rm -f
CFLAGS = -c

OBJS = \
  LzmaDecode.o \
  LzmaTest.o \
  decode.o

all: $(PROG)

$(PROG): $(OBJS)
	$(CC) -o $(PROG) $(LDFLAGS) $(OBJS) $(LIB)

LzmaDecode.o: LzmaDecode.c
	$(CC) $(CFLAGS) LzmaDecode.c

LzmaTest.o: LzmaTest.c
	$(CC) $(CFLAGS) LzmaTest.c

decode.o: decode.c decode.h
	$(CC) $(CFLAGS) decode.c

clean:
	-$(RM) $(PROG) $(OBJS)

