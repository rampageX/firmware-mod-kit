CC=gcc
CFLAGS=-g -Wall
TARGET=buffalo-enc

$(TARGET): buffalo-enc.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET).c *.o -o $(TARGET)

buffalo-enc.o:
	$(CC) $(CFLAGS) $(LDFLAGS) buffalo-lib.c -c

clean:
	rm -f buffalo-enc.o buffalo-lib.o $(TARGET)

distclean: clean
