CC=gcc
CFLAGS=-Wall
TARGET=crcalc

all: $(TARGET) crc32

$(TARGET): common.o patch.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET).c *.o -o $(TARGET)

crc32: crc.o
	$(CC) $(CFLAGS) $(LDFLAGS) crc32.c crc.o -o crc32 

common.o:
	$(CC) $(CFLAGS) $(LDFLAGS) common.c -c

patch.o: crc.o md5.o
	$(CC) $(CFLAGS) $(LDFLAGS) patch.c -c

crc.o:
	$(CC) $(CFLAGS) $(LDFLAGS) crc.c -c

md5.o:
	$(CC) $(CFLAGS) $(LDFLAGS) md5.c -c

clean:
	rm -f *.o $(TARGET) crc32
