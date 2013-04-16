CC=gcc
CFLAGS=-O2
TARGET=tpl-tool

$(TARGET): $(TARGET).o md5.o
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET).o md5.o -o $(TARGET)

$(TARGET).o: $(TARGET).c
	$(CC) $(CFLAGS) $(LDFLAGS) $(TARGET).c -c

md5.o:
	$(CC) $(CFLAGS) $(LDFLAGS) md5.c -c

clean:
	rm -f $(TARGET) *.o
