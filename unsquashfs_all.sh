#!/bin/bash
# Script to attempt to extract files from a SquashFS image using all of the available unsquashfs utilities in FMK until one is successful.
#
# Craig Heffner
# 27 August 2011
BINDIR=`dirname $0`
. "$BINDIR/common.inc"
IMG="$1"
DIR="$2"

ROOT="./src"
SUBDIRS="squashfs-2.1-r2 \
squashfs-3.0 \
squashfs-3.0-lzma-damn-small-variant \
others/squashfs-2.0-nb4 \
others/squashfs-3.0-e2100 \
others/squashfs-3.2-r2 \
others/squashfs-3.2-r2-lzma \
others/squashfs-3.2-r2-lzma/squashfs3.2-r2/squashfs-tools \
others/squashfs-3.2-r2-hg612-lzma \
others/squashfs-3.2-r2-wnr1000 \
others/squashfs-3.2-r2-rtn12 \
others/squashfs-3.3 \
others/squashfs-3.3-lzma/squashfs3.3/squashfs-tools \
others/squashfs-3.3-grml-lzma/squashfs3.3/squashfs-tools \
others/squashfs-3.4-cisco \
others/squashfs-3.4-nb4 \
others/squashfs-4.2-official \
others/squashfs-4.2 \
others/squashfs-4.0-lzma \
others/squashfs-4.0-realtek \
others/squashfs-hg55x-bin"
TIMEOUT="15"
MKFS=""
DEST=""

function wait_for_complete()
{
	I=0
	PNAME="$1"

	while [ $I -lt $TIMEOUT ]
	do
		sleep 1

		if [ "$(pgrep $PNAME)" == "" ]
		then
			break
		fi

		((I=$I+1))
	done

	if [ "$I" == "$TIMEOUT" ]
	then
		kill -9 $(pgrep $PNAME) 2>/dev/null
	fi
}

if [ "$IMG" == "" ] || [ "$IMG" == "-h" ]
then
	echo "Usage: $0 <squashfs image> [output directory]"
	exit 1
fi

if [ "$DIR" == "" ]
then
	BDIR="./squashfs-root"
	DIR=$BDIR
	I=1

	while [ -e $DIR ]
	do
		DIR=$BDIR-$I
		((I=$I+1))
	done
fi

IMG=$(readlink -f $IMG)
DIR=$(readlink -f $DIR)

# Make sure we're operating out of the FMK directory
cd $(dirname $(readlink -f $0))

DEST="-dest $DIR"
MAJOR=$(./src/binwalk-1.0/src/bin/binwalk-script -m ./src/binwalk-*/src/magic.binwalk -l 1 "$IMG" | head -4 | tail -1 | sed -e 's/.*version //' | cut -d'.' -f1)

echo -e "Attempting to extract SquashFS $MAJOR.X file system...\n"

for SUBDIR in $SUBDIRS
do
	if [ "$(echo $SUBDIR | grep "$MAJOR\.")" == "" ]
	then
		echo "Skipping $SUBDIR (wrong version)..."
		continue
	fi

	unsquashfs="$ROOT/$SUBDIR/unsquashfs"
	mksquashfs="$ROOT/$SUBDIR/mksquashfs"

	if [ -e $unsquashfs-lzma ]; then
		echo -ne "\nTrying $unsquashfs-lzma... "

		$unsquashfs-lzma $DEST $IMG 2>/dev/null &
		#sleep $TIMEOUT && kill $! 1>&2 >/dev/null
		wait_for_complete $unsquashfs-lzma
		
		if [ -d "$DIR" ]
                then
			if [ "$(ls $DIR)" != "" ]
			then
				# Most systems will have busybox - make sure it's a non-zero file size
				if [ -e "$DIR/bin/sh" ]
				then
					if [ "$(wc -c $DIR/bin/sh | cut -d' ' -f1)" != "0" ]
					then
						MKFS="$mksquashfs-lzma"
					fi
				else
                        		MKFS="$mksquashfs-lzma"
				fi
			fi
			
			if [ "$MKFS" == "" ]
			then
				rm -rf "$DIR"
			fi
                fi
	fi
	if [ "$MKFS" == "" ] && [ -e $unsquashfs ]; then
		echo -ne "\nTrying $unsquashfs... "

		$unsquashfs $DEST $IMG 2>/dev/null &
		#sleep $TIMEOUT && kill $! 1>&2 >/dev/null
		wait_for_complete $unsquashfs

		if [ -d "$DIR" ]
		then
			if [ "$(ls $DIR)" != "" ]
			then
				# Most systems will have busybox - make sure it's a non-zero file size
				if [ -e "$DIR/bin/sh" ]
				then
					if [ "$(wc -c $DIR/bin/sh | cut -d' ' -f1)" != "0" ]
					then
						MKFS="$mksquashfs"
					fi
				else
					MKFS="$mksquashfs"
				fi
			fi

			if [ "$MKFS" == "" ]
			then
				rm -rf "$DIR"
			fi
		fi
	fi	

	if [ "$MKFS" != "" ]
	then
		echo "File system sucessfully extracted!"
		echo "MKFS=\"$MKFS\""
		exit 0
	fi
done

echo "File extraction failed!"
exit 1
