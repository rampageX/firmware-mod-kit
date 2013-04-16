
# Copyright (C) 2006-2008 Junjiro Okajima
# Copyright (C) 2006-2008 Tomas Matejicek, slax.org
#
# LICENSE follows the described ones in lzma and squashfs.

# $Id: Makefile,v 1.30 2008-03-12 16:24:54 jro Exp $

# paths
Sqlzma = ${CURDIR}

LzmaVer =  
Lzma = ${Sqlzma}/${LzmaVer}
SqVer = squashfs3.3
Squashfs = ${Sqlzma}/${SqVer}
#KVer = linux-2.6.24.3
SqFs = ${Squashfs}/kernel-patches/${KVer}/fs/squashfs
#KDir = /lib/modules/$(shell uname -r)/build

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

########################################

#-include priv.mk

#ifdef BuildSquashfs
#CONFIG_SQUASHFS = m
#CONFIG_SQUASHFS_EMBEDDED =
#CONFIG_SQUASHFS_FRAGMENT_CACHE_SIZE = 3
#EXTRA_CFLAGS = -I${Sqlzma} -I${SqFs}/../../include -Wall -Werror
#EXTRA_CFLAGS += -DCONFIG_SQUASHFS_MODULE -UCONFIG_SQUASHFS
#EXTRA_CFLAGS += -UCONFIG_SQUASHFS_EMBEDDED -DCONFIG_SQUASHFS_FRAGMENT_CACHE_SIZE=3
# if you enabled CONFIG_PREEMPT, but want CPU to try concentrating
# the uncompression, then define UnsquashNoPreempt.
# if you don't define UnsquashNoPreempt, the behaviour follows
# the CONFIG_PREEMPT.
#EXTRA_CFLAGS += -DUnsquashNoPreempt
#export

# the environment variables are not inherited since 2.6.23
#MAKE += SQLZMA_EXTRA_CFLAGS="$(shell echo ${EXTRA_CFLAGS} | sed -e 's/\"/\\\\\\"/g')"
#
#all: all_sq
#
#FORCE:
#all_sq:
#	${MAKE} -C ${KDir} M=${SqFs} modules

#clean: clean_sq
#clean_sq:
#	${MAKE} -C ${KDir} M=${SqFs} clean
#endif

########################################
#
#load:
#	for i in ${LzmaC}/kmod/unlzma.ko ${LzmaC}/kmod/sqlzma.ko \
#		${SqFs}/squashfs.ko; \
#	do sudo insmod $$i; done
#
#unload:
#	-sudo rmmod squashfs sqlzma unlzma
