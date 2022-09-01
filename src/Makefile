CC := gcc
CXX := g++
INCLUDEDIR = .
CFLAGS := -I$(INCLUDEDIR) -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -O2

all: asustrx addpattern untrx motorola-bin splitter3 bffutils unjffs2
	$(MAKE) -C ./uncramfs/
	$(MAKE) -C ./uncramfs-lzma/
	$(MAKE) -C ./cramfs-2.x/
	$(MAKE) -C ./cramfsswap/
	$(MAKE) -C ./squashfs-2.1-r2/
	$(MAKE) -C ./squashfs-3.0/
	$(MAKE) -C ./squashfs-3.0-lzma-damn-small-variant/
	$(MAKE) -C ./wrt_vx_imgtool/
	$(MAKE) -C ./others/
	$(MAKE) -C ./crcalc/
	$(MAKE) -C ./webcomp-tools/
	$(MAKE) -C ./tpl-tool/src/
	$(MAKE) -C ./firmware-tools/
	$(MAKE) -C ./yaffs2utils/
	$(MAKE) -C ./jffs2
	$(MAKE) -C ./mountcp

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
	$(MAKE) -C ./bff/

unjffs2:
	$(MAKE) -C ./jffs2

clean:
	rm -f *.o
	rm -f motorola-bin
	rm -f untrx
	rm -f asustrx
	rm -f addpattern
	rm -f splitter3
	rm -f binwalk
	$(MAKE) -C ./jffs2 clean
	$(MAKE) -C ./squashfs-2.1-r2/ clean
	$(MAKE) -C ./squashfs-3.0 clean
	$(MAKE) -C ./squashfs-3.0-lzma-damn-small-variant clean
	$(MAKE) -C ./cramfs-2.x clean
	$(MAKE) -C ./uncramfs clean
	$(MAKE) -C ./uncramfs-lzma clean
	$(MAKE) -C ./cramfsswap clean
	$(MAKE) -C ./wrt_vx_imgtool clean
	$(MAKE) -C ./others clean
	$(MAKE) -C ./crcalc clean
	$(MAKE) -C ./webcomp-tools clean
	$(MAKE) -C ./firmware-tools/ clean
	$(MAKE) -C ./bff/ clean
	$(MAKE) -C ./yaffs2utils/ clean
	$(MAKE) -C ./jffs2 clean
	$(MAKE) -C ./mountcp clean

cleanall: clean
	rm -rf Makefile config.* *.cache

distclean: cleanall
