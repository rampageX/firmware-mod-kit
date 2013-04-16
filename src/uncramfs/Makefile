# 
# Makefile for uncramfs
#
# Copyright Andrew Stitcher, 2001
#
# Distributed under the terms of the GPL
#

.PHONY: clean dist

COFLAGS:=-r$(TAG)
CPPFLAGS:=-g -O
CFLAGS:=-g -O
LDLIBS:=-lz

#COFILES:=uncramfs.cc uncramfs.c cramfs.h Makefile VERSION README respin.sh uncramfs-w.pl
COFILES:=uncramfs.c cramfs.h Makefile VERSION README uncramfs-w.pl
TARGET:=uncramfs
DISTFILES=$(COFILES) $(TARGET)

include VERSION
VERSIONED_NAME=$(TARGET)-$(subst ",,$(VERSION))
DISTFILE=$(VERSIONED_NAME).tar.gz

all: $(TARGET)

dist: $(DISTFILE)

co: $(COFILES)

$(DISTFILE): $(DISTFILES)
	mkdir $(VERSIONED_NAME)
	cp $^ $(VERSIONED_NAME)
	tar cfz $(DISTFILE) $(VERSIONED_NAME)
	rm -rf $(VERSIONED_NAME)

clean:
	-rm $(TARGET)
	-rm $(DISTFILE)
	-rcsclean

#cuncramfs: uncramfs.c
#	$(CC) $(CFLAGS) $< -o $@ $(LDLIBS)

#uncramfs.cc: cramfs.h VERSION
uncramfs.c: cramfs.h VERSION
