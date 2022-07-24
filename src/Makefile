CC := gcc
CXX := g++
INCLUDEDIR = .
CFLAGS := -I$(INCLUDEDIR) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -O2

all: asustrx addpattern untrx motorola-bin splitter3 bffutils unjffs2
	make -C ./uncramfs/
	make -C ./uncramfs-lzma/
	make -C ./cramfs-2.x/
	make -C ./cramfsswap/
	make -C ./squashfs-2.1-r2/
	make -C ./squashfs-3.0/
	make -C ./squashfs-3.0-lzma-damn-small-variant/
	make -C ./wrt_vx_imgtool/
	make -C ./others/
	make -C ./crcalc/
	make -C ./webcomp-tools/
	make -C ./tpl-tool/src/
	make -C ./firmware-tools/
	make -C ./yaffs2utils/
	make -C ./jffs2
	make -C ./mountcp

addpattern: addpattern.o
	$(CC) addpattern.o -o $@

untrx: untrx.o
	$(CXX) untrx.o -o $@

splitter3: splitter3.o
	$(CXX) splitter3.o -o $@

asustrx: asustrx.o
	$(CC) asustrx.o -o $@

motorola-bin: motorola-bin.o
	$(CC) motorola-bin.o -o $@

bffutils:
	make -C ./bff/

unjffs2:
	make -C ./jffs2

clean:
	rm -f *.o
	rm -f motorola-bin
	rm -f untrx
	rm -f asustrx
	rm -f addpattern
	rm -f splitter3
	rm -f binwalk
	make -C ./jffs2 clean
	make -C ./squashfs-2.1-r2/ clean
	make -C ./squashfs-3.0 clean
	make -C ./squashfs-3.0-lzma-damn-small-variant clean
	make -C ./cramfs-2.x clean
	make -C ./uncramfs clean
	make -C ./uncramfs-lzma clean
	make -C ./cramfsswap clean
	make -C ./wrt_vx_imgtool clean
	make -C ./others clean
	make -C ./crcalc clean
	make -C ./webcomp-tools clean
	make -C ./firmware-tools/ clean
	make -C ./bff/ clean
	make -C ./yaffs2utils/ clean
	make -C ./jffs2 clean
	make -C ./mountcp clean

cleanall: clean
	rm -rf Makefile config.* *.cache

distclean: cleanall
