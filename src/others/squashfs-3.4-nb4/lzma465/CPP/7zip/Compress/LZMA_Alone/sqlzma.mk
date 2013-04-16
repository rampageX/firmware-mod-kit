
# Copyright (C) 2006-2007 Junjiro Okajima
# Copyright (C) 2006-2007 Tomas Matejicek, slax.org
#
# LICENSE follows the described one in lzma.txt.

# $Id: sqlzma.mk,v 1.1 2007-11-05 05:43:36 jro Exp $

ifndef Sqlzma
$(error Sqlzma is not defined)
endif

include makefile.gcc

ifdef UseDebugFlags
DebugFlags = -Wall -O0 -g -UNDEBUG
endif
# -pthread
CXXFLAGS = ${CFLAGS} -D_REENTRANT -include pthread.h -DNDEBUG ${DebugFlags}
Tgt = liblzma_r.a

all: ${Tgt}

RObjs = LzmaEncoder_r.o LzmaEnc_r.o Alloc_r.o StreamUtils_r.o \
	LzFind_r.o Bcj2Coder_r.o OutBuffer_r.o 7zCrc_r.o

%_r.cc: ../%.cpp
	ln $< $@
%_r.c: ../../../../C/%.c
	ln $< $@
%_r.c: ../../../../C/Compress/Lz/%.c
	ln $< $@
%_r.cc: ../../Common/%.cpp
	ln $< $@
%_r.cc: ../RangeCoder/%.cpp
	ln $< $@
LzmaEncoder_r.o: CXXFLAGS += -I../
LzmaEnc_r.o: CFLAGS += -I../../../../C
LzFind_r.o: CFLAGS += -I../../../../C
Alloc_r.o: CFLAGS += -I../../../../C
StreamUtils_r.o: CXXFLAGS += -I../../Common
# MatchFinder_r.o: CFLAGS += -I../../../../C/Compress/Lz
LzFindMt_r.o: CFLAGS += -I../../../../C
RangeCoderBit_r.o: CXXFLAGS += -I../RangeCoder
Bcj2Coder_r.o: CXXFLAGS += -I../
OutBuffer_r.o: CXXFLAGS += -I../../Common
7zCrc_r.o: CFLAGS += -I../../../../C

comp.o: CXXFLAGS += -I${Sqlzma}
comp.o: comp.cc ${Sqlzma}/sqlzma.h

liblzma_r.a: ${RObjs} comp.o
	${AR} cr $@ $^

clean: clean_sqlzma
clean_sqlzma:
	$(RM) comp.o *_r.o ${Tgt} *~

# Local variables: ;
# compile-command: (concat "make Sqlzma=../../../../.. -f " (file-name-nondirectory (buffer-file-name)));
# End: ;
