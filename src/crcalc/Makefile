CC=gcc
CFLAGS=-Wall
TARGET=crcalc

$(TARGET): patch.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET).c *.o -o $(TARGET)

patch.o: crc.o md5.o
	$(CC) $(CFLAGS) $(LDFLAGS) patch.c -c

crc.o:
	$(CC) $(CFLAGS) $(LDFLAGS) crc.c -c

md5.o:
	$(CC) $(CFLAGS) $(LDFLAGS) md5.c -c

clean:
	rm -f *.o $(TARGET)
