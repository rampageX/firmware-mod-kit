#!/usr/bin/make -f

all: cramfsswap strip
debian: cramfsswap

cramfsswap: cramfsswap.c
	gcc -Wall -g -O -o cramfsswap cramfsswap.c -lz

strip:
	strip cramfsswap

install: cramfsswap
	install cramfsswap $(DESTDIR)/usr/bin
	
clean:	
	rm -f cramfsswap
