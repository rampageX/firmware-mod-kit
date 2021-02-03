#!/bin/bash
BINDIR=`dirname $0`
. "$BINDIR/common.inc"

DIR="$1"
NEXT_PARAM=""

if [ "$1" == "-h" ]; then
	echo "Usage: $0 [FMK directory] [-nopad | -min]"
	exit 1
fi

if [ "$DIR" == "" ] || [ "$DIR" == "-nopad" ] || [ "$DIR" == "-min" ]; then
	DIR="fmk"
	NEXT_PARAM="$1"
else
	NEXT_PARAM="$2"
fi

# Need to extract file systems as ROOT
if [ "$UID" != "0" ]; then
        SUDO="sudo"
else
        SUDO=""
fi

DIR=$(readlink -f $DIR)

# Make sure we're operating out of the FMK directory
cd $(dirname $(readlink -f $0))

# Order matters here!
eval $(cat shared-ng.inc)
eval $(cat $CONFLOG)
FSOUT="$DIR/new-filesystem.$FS_TYPE"

printf "Firmware Mod Kit (build) ${VERSION}, (c)2011-2013 Craig Heffner, Jeremy Collake\n\n"

if [ ! -d "$DIR" ]; then
	echo -e "Usage: $0 [build directory] [-nopad]\n"
	exit 1
fi

# Always try to rebuild, let make decide if necessary
Build_Tools

echo "Building new $FS_TYPE file system... (this may take several minutes!)"

# Clean up any previously created files
rm -rf "$FWOUT" "$FSOUT"

MKFS_ARGS="-all-root"

# Build the appropriate file system
case $FS_TYPE in
	"squashfs")
		# Check for squashfs 4.0 realtek, which requires the -comp option to build lzma images.
		if [ "$FS_COMPRESSION" == "xz" ]; then
            COMP="-comp xz"
		elif [ "$FS_COMPRESSION" == "lzma" ]; then
			if [ "$(echo $MKFS | grep 'squashfs-4.0-realtek')" != "" ] || [ "$(echo $MKFS | grep 'squashfs-4.2')" != "" ]; then
				COMP="-comp lzma"
			else
				COMP=""
			fi
		elif [ "$FS_COMPRESSION" == "xz" ]; then
			if [ "$(echo $MKFS | grep 'squashfs-4.0-realtek')" != "" ] || [ "$(echo $MKFS | grep 'squashfs-4.2')" != "" ]; then
				COMP="-comp xz"
			else
				COMP=""
			fi

			if [ "$COMP_XZ_ALL_ROOT" = "" ]; then
				MKFS_ARGS=""
			fi

			if [ "$COMP_XZ_XATTRS" != "" ]; then
				MKFS_ARGS="$MKFS_ARGS $COMP_XZ_XATTRS"
			fi

			MKFS_ARGS="$MKFS_ARGS $FS_ARGS"
		fi

		# Mksquashfs 4.0 tools don't support the -le option; little endian is built by default
		if [ "$(echo $MKFS | grep 'squashfs-4.')" != "" ] && [ "$ENDIANESS" == "-le" ];	then
			ENDIANESS=""
		fi
		
		# Increasing the block size minimizes the resulting image size (larger dictionary). Max block size of 1MB.
		if [ "$NEXT_PARAM" == "-min" ];	then
			echo "Blocksize override (-min). Original used $((FS_BLOCKSIZE/1024))KB blocks. New firmware uses 1MB blocks."
			FS_BLOCKSIZE="$((1024*1024))"
		fi

		# if blocksize var exists, then add '-b' parameter
                if [ "$FS_BLOCKSIZE" != "" ]; then
			BS="-b $FS_BLOCKSIZE"
			HR_BLOCKSIZE="$(($FS_BLOCKSIZE/1024))"
			echo "Squashfs block size is $HR_BLOCKSIZE Kb"
		fi

		$SUDO $MKFS "$ROOTFS" "$FSOUT" $ENDIANESS $BS $COMP $MKFS_ARGS
		;;
	"cramfs")
		$SUDO $MKFS "$ROOTFS" "$FSOUT"
		if [ "$ENDIANESS" == "-be" ]; then
			mv "$FSOUT" "$FSOUT.le"
			./src/cramfsswap/cramfsswap "$FSOUT.le" "$FSOUT"
			rm -f "$FSOUT.le"
		fi
		;;
	"yaffs")
		echo "WARNING: YAFFS2 completely untested !! Hit any key to confirm ..."
		$SUDO $MKFS "$ROOTFS" "$FSOUT"
		pause
		;;
	"jffs2")
		if [ "$ENDIANESS" == "-le" ]; then
			echo "Building JFFS2 file system (little endian) ..."
			$SUDO $MKFS -r "$ROOTFS" -o "$FSOUT" --little-endian

		elif [ "$ENDIANESS" == "-be" ]; then
			echo "Building JFFS2 file system (big endian) ..."
			$SUDO $MKFS -r "$ROOTFS" -o "$FSOUT" --big-endian
		fi
		;;
	*)
		echo "Unsupported file system '$FS_TYPE'!"
		;;
esac

if [ ! -e $FSOUT ]; then
	echo "Failed to create new file system! Quitting..."
	exit 1
fi

# Append the new file system to the first part of the original firmware file
cp $HEADER_IMAGE $FWOUT
$SUDO cat $FSOUT >> $FWOUT

# Calculate and create any filler bytes required between the end of the file system and the footer / EOF.
CUR_SIZE=$(ls -l $FWOUT | awk '{print $5}')
((FILLER_SIZE=$FW_SIZE-$CUR_SIZE-$FOOTER_SIZE))

if [ "$FILLER_SIZE" -lt 0 ]; then
	echo "ERROR: New firmware image will be larger than original image!"
	echo "       Building firmware images larger than the original can brick your device!"
	echo "       Try re-running with the -min option, or remove any unnecessary files."
	echo "       REFUSING to create new firmware image."
	echo ""
	echo "       Original file size: $FW_SIZE"
	echo "       Current file size:  $CUR_SIZE (plus footer of $FOOTER_SIZE bytes)"
	echo ""
	echo "       Quitting..."
#	rm -f "$FWOUT" "$FSOUT"
	exit 1
else
	if [ "$NEXT_PARAM" != "-nopad" ]; then
		echo "Remaining free bytes in firmware image: $FILLER_SIZE"
		perl -e "print \"\xFF\"x$FILLER_SIZE" >> "$FWOUT"
	else
		echo "Padding of firmware image disabled via -nopad"
	fi	
fi

# Append the footer to the new firmware image, if there is any footer
if [ "$FOOTER_SIZE" -gt "0" ]; then
	echo "Appending ${FOOTER_SIZE} byte footer at offset ${FOOTER_OFFSET}"
	cat $FOOTER_IMAGE >> "$FWOUT"
fi

CHECKSUM_ERROR=0

# Calculate new checksum values for the firmware header
# trx, dlob, uimage
# Buffalo and some other post-processors obfuscate these images
# so we must akways try prior to vendor processing below
./src/crcalc/crcalc "$FWOUT" "$BINLOG"
if [ $? -ne 0 ]; then		
	CHECKSUM_ERROR=1
fi

# Vendor specific post-processing
# For some images, will apply checksum (TP-Link)
# Others, will encrypt image (Buffalo)
case $HEADER_TYPE in
	"tp-link")
		printf "\nCorrecting TP-Link firmware image ... "
		src/tpl-tool/src/tpl-tool -x "$FWOUT"
		if [ $? -ne 0 ]; then				
			CHECKSUM_ERROR=1
			printf "ERROR"
		else		
			src/tpl-tool/src/tpl-tool -b "$FWOUT"
			mv "$FWOUT-new" "$FWOUT"
			src/tpl-tool/src/tpl-tool -s "$FWOUT"
			if [ $? -ne 0 ]; then		
				CHECKSUM_ERROR=1
				printf "ERROR"				
			else
				printf "Done"
				CHECKSUM_ERROR=0
			fi
		fi
		rm -f "$FWOUT-header" "$FWOUT-kernel" "$FWOUT-rootfs"
		;;
	"buffalo")
		printf "\nEncrypting Buffalo firmware image ... "
		src/firmware-tools/buffalo-enc -i "$FWOUT" -o "$FWOUT.enc"
		if [ $? -ne 0 ]; then					
			CHECKSUM_ERROR=1
			printf "ERROR"
		else
			printf "saved as $FWOUT.enc"
		fi
		;;
	*)
	;;
esac

if [ $CHECKSUM_ERROR -eq 0 ]; then
	printf "\nFinished! "
else
	printf "\nFirmware header not supported; firmware checksums may be incorrect. "
fi

if [ -e "$FSOUT" ]; then
	rm -f "$FSOUT"
fi

printf "\nNew firmware image has been saved to: $FWOUT\n"
