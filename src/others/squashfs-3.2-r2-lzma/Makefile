
# Copyright (C) 2006, 2007 Junjiro Okajima
# Copyright (C) 2006, 2007 Tomas Matejicek, slax.org
#
# LICENSE follows the described ones in lzma and squashfs.

# $Id: Makefile,v 1.2 2007-10-26 07:25:05 steven Exp $

# paths
Sqlzma = ${CURDIR}
#LzmaVer = lzma443
LzmaVer = 
Lzma = ${Sqlzma}/${LzmaVer}
SqVer = squashfs3.2-r2
Squashfs = ${Sqlzma}/${SqVer}

#LzmaC = ${Lzma}/C/7zip/Compress/LZMA_C
#LzmaAlone = ${Lzma}/C/7zip/Compress/LZMA_Alone
LzmaC = ${Lzma}/C/Compress/Lzma
LzmaAlone = ${Lzma}/CPP/7zip/Compress/LZMA_Alone
SqTools = ${Squashfs}/squashfs-tools

# enable it if you want to add -g option when compiling
UseDebugFlags =
# disable it if you don't want to compile squashfs kernel module here
BuildSquashfs = 1

export

all:
	${MAKE} -C ${LzmaC} -f sqlzma.mk $@
	${MAKE} -C ${LzmaAlone} -f sqlzma.mk $@
	${MAKE} -C ${SqTools} $@

clean:
	${MAKE} -C ${LzmaC} -f sqlzma.mk $@
	${MAKE} -C ${LzmaAlone} -f sqlzma.mk $@
	${MAKE} -C ${SqTools} $@
	${RM} *~

install:
	#cp squashfs3.2-r2/squashfs-tools/mksquashfs /opt/buildroot-gdb/bin/mksquashfs_lzma-3.2
	cp squashfs3.2-r2/squashfs-tools/mksquashfs ../../source/vendors/Ralink/RT2880/mksquashfs_lzma-3.2
 
########################################



