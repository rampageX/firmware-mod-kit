#
# build a zlib compatible wrapper around p7zip (davidm@snapgear.com)
#

PROG = liblzma.a

P7ZIP_SRC = p7zip

# CXX = g++ -O3 -Wall
AR ?= ar
RM ?= rm -f
CFLAGS += -c -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -DNDEBUG -D_REENTRANT -DENV_UNIX -DEXCLUDE_COM -DNO_REGISTRY -DFORMAT_7Z -DFORMAT_BZIP2 -DFORMAT_ZIP -DFORMAT_TAR -DFORMAT_GZIP -DCOMPRESS_LZMA -DCOMPRESS_BCJ_X86 -DFORMAT_SPLIT -DFORMAT_Z -DCOMPRESS_BCJ2 -DCOMPRESS_COPY -DCOMPRESS_MF_MT -DCOMPRESS_PPMD -DCOMPRESS_DEFLATE -DCOMPRESS_DEFLATE64 -DCOMPRESS_IMPLODE -DCOMPRESS_BZIP2 -DCRYPTO_ZIP -DCRYPTO_7ZAES -DCRYPTO_AES -c -I$(P7ZIP_SRC)/myWindows -I$(P7ZIP_SRC) -I$(P7ZIP_SRC)/include_windows 


MY_WINDOWS=

include $(P7ZIP_SRC)/makefile.machine

OBJS = \
  ZLib.o \
  LZMADecoder.o \
  LZMAEncoder.o \
  LZInWindow.o \
  LZOutWindow.o \
  RangeCoderBit.o \
  InBuffer.o \
  OutBuffer.o \
  FileStreams.o \
  Alloc.o \
  StreamUtils.o \
  CommandLineParser.o \
  Synchronization.o \
  MT.o \
  CRC.o \
  String.o \
  StringConvert.o \
  StringToInt.o \
  Vector.o \


all: $(PROG)

$(PROG): $(OBJS)
	$(AR) r $(PROG) $(OBJS)

ZLib.o: liblzma/ZLib.cpp
	$(CXX) $(CFLAGS) liblzma/ZLib.cpp

LZMADecoder.o: $(P7ZIP_SRC)/7zip/Compress/LZMA/LZMADecoder.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/7zip/Compress/LZMA/LZMADecoder.cpp

LZMAEncoder.o: $(P7ZIP_SRC)/7zip/Compress/LZMA/LZMAEncoder.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/7zip/Compress/LZMA/LZMAEncoder.cpp

Synchronization.o: $(P7ZIP_SRC)/Windows/Synchronization.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/Windows/Synchronization.cpp

StreamUtils.o: $(P7ZIP_SRC)/7zip/Common/StreamUtils.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/7zip/Common/StreamUtils.cpp

MT.o: $(P7ZIP_SRC)/7zip/Compress/LZ/MT/MT.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/7zip/Compress/LZ/MT/MT.cpp

LZInWindow.o: $(P7ZIP_SRC)/7zip/Compress/LZ/LZInWindow.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/7zip/Compress/LZ/LZInWindow.cpp

LZOutWindow.o: $(P7ZIP_SRC)/7zip/Compress/LZ/LZOutWindow.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/7zip/Compress/LZ/LZOutWindow.cpp

RangeCoderBit.o: $(P7ZIP_SRC)/7zip/Compress/RangeCoder/RangeCoderBit.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/7zip/Compress/RangeCoder/RangeCoderBit.cpp

InBuffer.o: $(P7ZIP_SRC)/7zip/Common/InBuffer.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/7zip/Common/InBuffer.cpp

OutBuffer.o: $(P7ZIP_SRC)/7zip/Common/OutBuffer.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/7zip/Common/OutBuffer.cpp

FileStreams.o: $(P7ZIP_SRC)/7zip/Common/FileStreams.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/7zip/Common/FileStreams.cpp

Alloc.o: $(P7ZIP_SRC)/Common/Alloc.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/Common/Alloc.cpp

C_FileIO.o: $(P7ZIP_SRC)/Common/C_FileIO.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/Common/C_FileIO.cpp

CommandLineParser.o: $(P7ZIP_SRC)/Common/CommandLineParser.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/Common/CommandLineParser.cpp

CRC.o: $(P7ZIP_SRC)/Common/CRC.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/Common/CRC.cpp

MyWindows.o: $(P7ZIP_SRC)/Common/MyWindows.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/Common/MyWindows.cpp

String.o: $(P7ZIP_SRC)/Common/String.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/Common/String.cpp

StringConvert.o: $(P7ZIP_SRC)/Common/StringConvert.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/Common/StringConvert.cpp

StringToInt.o: $(P7ZIP_SRC)/Common/StringToInt.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/Common/StringToInt.cpp

Vector.o: $(P7ZIP_SRC)/Common/Vector.cpp
	$(CXX) $(CFLAGS) $(P7ZIP_SRC)/Common/Vector.cpp

clean:
	-$(RM) $(PROG) $(OBJS)

romfs:
