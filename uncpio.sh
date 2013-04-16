#!/bin/bash

FSIMG="$1"
ROOTFS="$2"
ROOTFS_CREATED=0

if [ "$FSIMG" == "" ] || [ "$FSIMG" == "-h" ]
then
	echo "Usage: $(basename $0) <cpio archive> [output directory]\n"
	exit 1
fi

if [ "$ROOTFS" == "" ]
then
	ROOTFS="./cpio-root"
fi

FSIMG=$(readlink -f $FSIMG)
ROOTFS=$(readlink -f $ROOTFS)

if [ ! -e $ROOTFS ]
then
	mkdir -p $ROOTFS
	ROOTFS_CREATED=1
fi

cd $ROOTFS && cpio -i --no-absolute-filenames < $FSIMG

if [ "$(ls $ROOTFS)" == "" ] && [ "$ROOTFS_CREATED" == "1" ]
then
	rm -rf $ROOTFS
fi
