
# Copyright (C) 2006 Junjiro Okajima
# Copyright (C) 2006 Tomas Matejicek, slax.org
#
# LICENSE follows the described one in lzma.txt.

# $Id: sqlzma.mk,v 1.1.1.1 2007-11-22 06:05:47 steven Exp $

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

RObjs = LZMAEncoder_r.o Alloc_r.o LZInWindow_r.o CRC_r.o StreamUtils_r.o \
	OutBuffer_r.o RangeCoderBit_r.o
%_r.cc: ../LZMA/%.cpp
	ln $< $@
%_r.cc: ../LZ/%.cpp
	ln $< $@
%_r.cc: ../RangeCoder/%.cpp
	ln $< $@
%_r.cc: ../../Common/%.cpp
	ln $< $@
%_r.cc: ../../../Common/%.cpp
	ln $< $@
LZMAEncoder_r.o: CXXFLAGS += -I../LZMA
LZInWindow_r.o: CXXFLAGS += -I../LZ
RangeCoderBit_r.o: CXXFLAGS += -I../RangeCoder
OutBuffer_r.o StreamUtils_r.o: CXXFLAGS += -I../../Common
Alloc_r.o CRC_r.o: CXXFLAGS += -I../../../Common

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
