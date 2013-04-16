#!/bin/bash

DIR="$1"
NEXT_PARAM=""

if [ "$1" == "-h" ]
then
	echo "Usage: $0 [FMK directory] [-nopad | -min]"
	exit 1
fi

if [ "$DIR" == "" ] || [ "$DIR" == "-nopad" ] || [ "$DIR" == "-min" ]
then
	DIR="fmk"
	NEXT_PARAM="$1"
else
	NEXT_PARAM="$2"
fi

# Need to extract file systems as ROOT
if [ "$UID" != "0" ]
then
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

printf "Firmware Mod Kit (build-ng) ${VERSION}, (c)2011-2013 Craig Heffner, Jeremy Collake\n\n"

if [ ! -d "$DIR" ]
then
	echo -e "Usage: $0 [build directory] [-nopad]\n"
	exit 1
fi

# Check if FMK has been built, and if not, build it
if [ ! -e "./src/crcalc/crcalc" ]
then
	echo "Firmware-Mod-Kit has not been built yet. Building..."
	cd src && ./configure && make

	if [ $? -eq 0 ]
	then
		cd -
	else
		echo "Build failed! Quitting..."
		exit 1
	fi
fi

echo "Building new $FS_TYPE file system..."

# Clean up any previously created files
rm -rf "$FWOUT" "$FSOUT"

# Build the appropriate file system
case $FS_TYPE in
	"squashfs")
		# Check for squashfs 4.0 realtek, which requires the -comp option to build lzma images.
		if [ "$(echo $MKFS | grep 'squashfs-4.0-realtek')" != "" ] && [ "$FS_COMPRESSION" == "lzma" ]
		then
			COMP="-comp lzma"
		else
			COMP=""
		fi

		# Mksquashfs 4.0 tools don't support the -le option; little endian is built by default
		if [ "$(echo $MKFS | grep 'squashfs-4.')" != "" ] && [ "$ENDIANESS" == "-le" ]
		then
			ENDIANESS=""
		fi
		
		# Increasing the block size minimizes the resulting image size (larger dictionary). Max block size of 1MB.
		if [ "$NEXT_PARAM" == "-min" ]
		then
			echo "Blocksize override (-min). Original used $((FS_BLOCKSIZE/1024))KB blocks. New firmware uses 1MB blocks."
			FS_BLOCKSIZE="$((1024*1024))"
		fi

		# if blocksize var exists, then add '-b' parameter
                if [ "$FS_BLOCKSIZE" != "" ]
		then
			BS="-b $FS_BLOCKSIZE"
		fi

		$SUDO $MKFS "$ROOTFS" "$FSOUT" $ENDIANESS $BS $COMP -all-root
		;;
	"cramfs")
		$SUDO $MKFS "$ROOTFS" "$FSOUT"
		if [ "$ENDIANESS" == "-be" ]
		then
			mv "$FSOUT" "$FSOUT.le"
			./src/cramfsswap/cramfsswap "$FSOUT.le" "$FSOUT"
			rm -f "$FSOUT.le"
		fi
		;;
	*)
		echo "Unsupported file system '$FS_TYPE'!"
		;;
esac

if [ ! -e $FSOUT ]
then
	echo "Failed to create new file system! Quitting..."
	exit 1
fi

# Append the new file system to the first part of the original firmware file
cp $HEADER_IMAGE $FWOUT
$SUDO cat $FSOUT >> $FWOUT

# Calculate and create any filler bytes required between the end of the file system and the footer / EOF.
CUR_SIZE=$(ls -l $FWOUT | awk '{print $5}')
((FILLER_SIZE=$FW_SIZE-$CUR_SIZE-$FOOTER_SIZE))

if [ "$FILLER_SIZE" -lt 0 ]
then
	echo "ERROR: New firmware image will be larger than original image!"
	echo "       Building firmware images larger than the original can brick your device!"
	echo "       Try re-running with the -min option, or remove any unnecessary files from the file system."
	echo "       Refusing to create new firmware image."
	echo ""
	echo "       Original file size: $FW_SIZE"
	echo "       Current file size:  $CUR_SIZE"
	echo ""
	echo "       Quitting..."
	rm -f "$FWOUT" "$FSOUT"
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
if [ "$FOOTER_SIZE" -gt "0" ]
then
	cat $FOOTER_IMAGE >> "$FWOUT"
fi

# Calculate new checksum values for the firmware header
# trx, dlob, uimage
./src/crcalc/crcalc "$FWOUT" "$BINLOG"

if [ $? -eq 0 ]
then
	echo -n "Finished! "
else
	echo -n "Firmware header not supported; firmware checksums may be incorrect. "
fi

# if a Buffalo image, then run encrypter - base on image name
if [ "$(echo $FWOUT | grep -i 'buffalo')" != "" ]
then	
	# product name, version, key, encryption type can be specified here
	$KEY="" # specify full param, e.g. -k mykey
	$MAGIC=""
	$PRODUCT=""
	$LONGSTATE=""
	./src/firmware-tools/buffalo-enc -i $FWOUT -o $FWOUT.buffalo.enc $KEY $MAGIC $PRODUCT $LONGSTATE
	#if [ $? -eq 0 ]
	#then
	#	echo "Encrypted Buffalo image created."
	#else
	#	echo "ERROR creating an encrypted Buffalo image"
	#fi
fi

echo "New firmware image has been saved to: $FWOUT"
